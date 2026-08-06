// Copyright The Signal. All Rights Reserved.

#include "ZP_VentDoor.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"

AZP_VentDoor::AZP_VentDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(Root);
	InteractionVolume->SetBoxExtent(AZP_InteractionVolumeExtent);
	InteractionVolume->SetRelativeLocation(AZP_InteractionVolumeOffset);
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	HingePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HingePivot"));
	HingePivot->SetupAttachment(Root);
	HingePivot->SetRelativeLocation(AZP_HingeOffset);

	VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
	VentMesh->SetupAttachment(HingePivot);
	VentMesh->SetRelativeLocation(-AZP_HingeOffset);
	VentMesh->SetCollisionProfileName(TEXT("BlockAll"));
	VentMesh->SetGenerateOverlapEvents(false);
	// Dynamic nav obstacle (2026-08-05 nav-aware doors): closed grate carves; FinishOpen's
	// NoCollision drops its nav contribution automatically.
	VentMesh->SetCanEverAffectNavigation(true);
}

void AZP_VentDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyHingeLayout();
}

void AZP_VentDoor::ApplyHingeLayout()
{
	if (HingePivot)
	{
		HingePivot->SetRelativeLocation(AZP_HingeOffset);
		HingePivot->SetRelativeRotation(FRotator::ZeroRotator);
	}
	if (VentMesh)
	{
		VentMesh->SetRelativeLocation(-AZP_HingeOffset);
	}
	if (InteractionVolume)
	{
		InteractionVolume->SetBoxExtent(AZP_InteractionVolumeExtent);
		InteractionVolume->SetRelativeLocation(AZP_InteractionVolumeOffset);
	}
}

void AZP_VentDoor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionVolume)
	{
		InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_VentDoor::OnOverlapBegin);
		InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_VentDoor::OnOverlapEnd);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] VentDoor %s: Ready — RequiredItem=%s, AZP_HingeOffset=%s, AZP_OpenRotation=%s"),
		*GetName(),
		*AZP_RequiredItemName.ToString(),
		*AZP_HingeOffset.ToString(),
		*AZP_OpenRotation.ToString());
}

// --- IZP_Interactable ---

FText AZP_VentDoor::GetInteractionPrompt_Implementation()
{
	// Without a cached character reference, return the format with the item name.
	// Per-overlap prompts in OnOverlapBegin pick the right one based on inventory state.
	return FText::Format(AZP_UnlockedPromptFormat, AZP_RequiredItemName);
}

void AZP_VentDoor::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bIsOpen || bIsAnimating)
	{
		return;
	}

	if (CheckPlayerHasItem(Interactor))
	{
		if (bAZP_ConsumeItemOnUse)
		{
			ConsumeItem(Interactor);
		}
		StartOpen();

		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Interactor->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->HideInteractionPrompt();
		}
	}
	else
	{
		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Interactor->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(AZP_LockedPrompt);
		}
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] VentDoor %s: Player missing %s"),
			*GetName(), *AZP_RequiredItemName.ToString());
	}
}

// --- Overlap (CardReader pattern) ---

void AZP_VentDoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace || bIsOpen) return;

	Grace->SetCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		const FText Prompt = CheckPlayerHasItem(Grace)
			? FText::Format(AZP_UnlockedPromptFormat, AZP_RequiredItemName)
			: AZP_LockedPrompt;
		PC->HUDWidget->ShowInteractionPrompt(Prompt);
	}
}

void AZP_VentDoor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

// --- Open animation ---

void AZP_VentDoor::StartOpen()
{
	if (bIsOpen || bIsAnimating) return;
	bIsAnimating = true;
	AnimT = 0.f;
	SetActorTickEnabled(true);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] VentDoor %s: StartOpen"), *GetName());
}

void AZP_VentDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsAnimating || !HingePivot) return;

	AnimT += (AZP_OpenDuration > KINDA_SMALL_NUMBER) ? (DeltaTime / AZP_OpenDuration) : 1.f;
	const float T = FMath::Clamp(AnimT, 0.f, 1.f);
	const float Eased = FMath::SmoothStep(0.f, 1.f, T);
	HingePivot->SetRelativeRotation(FMath::Lerp(FRotator::ZeroRotator, AZP_OpenRotation, Eased));

	if (T >= 1.f)
	{
		FinishOpen();
	}
}

void AZP_VentDoor::FinishOpen()
{
	bIsOpen = true;
	bIsAnimating = false;
	SetActorTickEnabled(false);

	if (VentMesh)
	{
		VentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (InteractionVolume)
	{
		InteractionVolume->SetGenerateOverlapEvents(false);
		InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] VentDoor %s: Open complete — collision cleared"), *GetName());
}

// --- Inventory access (Moonville reflection, mirrors CardReaderPanel) ---

UActorComponent* AZP_VentDoor::GetMoonvilleInventoryComp(ACharacter* Character)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Character);
	return Grace ? Grace->MoonvilleInventoryComp.Get() : nullptr;
}

bool AZP_VentDoor::HasItemInSlotArray(UActorComponent* InvComp, const FName& ArrayName, UObject* TargetDA)
{
	if (!InvComp || !TargetDA) return false;

	FProperty* SlotsProp = InvComp->GetClass()->FindPropertyByName(ArrayName);
	if (!SlotsProp) return false;

	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp) return false;

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(InvComp));
	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner) return false;

	FProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemProp = *It;
			break;
		}
	}
	if (!ItemProp) return false;

	FObjectProperty* ObjProp = CastField<FObjectProperty>(ItemProp);
	if (!ObjProp) return false;

	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
		if (SlotItem == TargetDA) return true;
	}
	return false;
}

bool AZP_VentDoor::CheckPlayerHasItem(ACharacter* Character)
{
	UObject* TargetDA = AZP_RequiredItemDA.LoadSynchronous();
	if (!TargetDA) return false;

	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);
	if (!InvComp) return false;

	if (HasItemInSlotArray(InvComp, FName("ItemSlots"), TargetDA)) return true;
	if (HasItemInSlotArray(InvComp, FName("ShortcutSlots"), TargetDA)) return true;
	return false;
}

void AZP_VentDoor::ConsumeItem(ACharacter* Character)
{
	UObject* TargetDA = AZP_RequiredItemDA.LoadSynchronous();
	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);
	if (!TargetDA || !InvComp) return;

	UFunction* RemoveFunc = InvComp->FindFunction(FName("RemoveItemByDataAsset"));
	if (RemoveFunc)
	{
		struct { UObject* AZP_ItemDataAsset; int32 AmountToRemove; } Params;
		Params.AZP_ItemDataAsset = TargetDA;
		Params.AmountToRemove = 1;
		InvComp->ProcessEvent(RemoveFunc, &Params);
	}
}
