// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitPanel.h"
#include "ZP_TransitMenuWidget.h"
#include "ZP_TransitSubsystem.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "ZP_ObjectiveSubsystem.h"
#include "ZP_NoteSubsystem.h"

AZP_TransitPanel::AZP_TransitPanel()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PanelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PanelMesh"));
	PanelMesh->SetupAttachment(Root);
	PanelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	InteractionVolume->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	// Default to the fully-C++ transit menu so a placed panel works with no WBP. Override in BP for custom visuals.
	TransitMenuWidgetClass = UZP_TransitMenuWidget::StaticClass();
}

void AZP_TransitPanel::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_TransitPanel::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_TransitPanel::OnOverlapEnd);
}

FText AZP_TransitPanel::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_TransitPanel::OnInteract_Implementation(ACharacter* Interactor)
{
	if (!TransitMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitPanel %s: No TransitMenuWidgetClass set!"), *GetName());
		return;
	}

	APlayerController* PC = Interactor ? Cast<APlayerController>(Interactor->GetController()) : nullptr;
	if (!PC) return;

	UUserWidget* Menu = CreateWidget<UUserWidget>(PC, TransitMenuWidgetClass);
	if (!Menu) return;

	Menu->AddToViewport(100);

	CurrentUser = Interactor;

	TArray<FZP_TransitMenuEntry> EntryList;
	BuildMenuEntries(EntryList);

	if (UZP_TransitMenuWidget* TransitMenu = Cast<UZP_TransitMenuWidget>(Menu))
	{
		TransitMenu->InitTransitMenu(this, EntryList);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitPanel %s: Widget does not extend UZP_TransitMenuWidget"), *GetName());
	}

	// Hand to Grace: switches to UI-only input, focuses the widget, watches for close to restore game input.
	if (AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Interactor))
	{
		Grace->OpenSaveMenu(Menu);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitPanel %s: Opened transit menu (%d destinations)"),
		*GetName(), Destinations.Num());
}

void AZP_TransitPanel::BuildMenuEntries(TArray<FZP_TransitMenuEntry>& OutEntries) const
{
	OutEntries.Reset();
	ACharacter* User = CurrentUser.Get();

	for (int32 i = 0; i < Destinations.Num(); ++i)
	{
		const FZP_TransitDestination& D = Destinations[i];
		const bool bAvail = IsDestinationAvailable(D, User);

		// HiddenUntilKnown: omit locked destinations entirely until they become available.
		if (!bAvail && D.LockStyle == EZP_TransitLockStyle::HiddenUntilKnown) continue;

		FZP_TransitMenuEntry Entry;
		Entry.DestinationIndex = i;
		Entry.DisplayName = D.DisplayName;
		Entry.bAvailable = bAvail;
		Entry.LockedReason = D.LockedReason;
		OutEntries.Add(Entry);
	}
}

bool AZP_TransitPanel::IsDestinationAvailable(const FZP_TransitDestination& Dest, ACharacter* ForCharacter) const
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;

	// --- Objective gate: the named objective must be complete. ---
	if (!Dest.RequiredObjectiveId.IsNone())
	{
		bool bComplete = false;
		if (GI)
		{
			if (UZP_ObjectiveSubsystem* Obj = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
			{
				bComplete = Obj->IsObjectiveComplete(Dest.RequiredObjectiveId);
			}
		}
		if (!bComplete) return false;
	}

	// --- Key/note gate: the player must have collected the note (NoteSubsystem). ---
	// (A collected note is stored under FName(DataAsset->GetPathName()), which equals the soft-path string.)
	if (!Dest.RequiredKeyItem.IsNull())
	{
		const FName ItemId(*Dest.RequiredKeyItem.ToSoftObjectPath().ToString());
		bool bHas = false;
		if (GI)
		{
			if (UZP_NoteSubsystem* Notes = GI->GetSubsystem<UZP_NoteSubsystem>())
			{
				bHas = Notes->HasNote(ItemId);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] Transit '%s' key/note gate: id=%s has=%d"),
			*Dest.DestinationId.ToString(), *ItemId.ToString(), bHas ? 1 : 0);
		// NOTE: real Moonville inventory keycards (not notes) would also be checked here — see M3.
		if (!bHas) return false;
	}

	return true;
}

void AZP_TransitPanel::TravelToDestination(int32 DestinationIndex)
{
	if (!Destinations.IsValidIndex(DestinationIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitPanel %s: invalid destination index %d"), *GetName(), DestinationIndex);
		return;
	}

	const FZP_TransitDestination& D = Destinations[DestinationIndex];

	if (!IsDestinationAvailable(D, CurrentUser.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] TransitPanel %s: destination %s not available"), *GetName(), *D.DestinationId.ToString());
		return;
	}

	if (D.TargetLevel.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] TransitPanel %s: destination %s has no TargetLevel"), *GetName(), *D.DestinationId.ToString());
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (GI)
	{
		if (UZP_TransitSubsystem* Sub = GI->GetSubsystem<UZP_TransitSubsystem>())
		{
			Sub->TravelTo(D.TargetLevel, D.ArrivalPointTag);
		}
	}
}

void AZP_TransitPanel::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->SetCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(PromptText);
	}
}

void AZP_TransitPanel::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->HideInteractionPrompt();
	}
}
