// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveContainer.h"
#include "ZP_ContainerUtils.h"
#include "ZP_LockableDoor.h"
#include "ZP_InteractDoor.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "ZP_ObjectiveSubsystem.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"

AZP_ObjectiveContainer::AZP_ObjectiveContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	ContainerMesh->SetupAttachment(Root);
	ContainerMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Child of the mesh so it inherits the mesh's scale — FitBoxToMeshBounds resizes it at BeginPlay.
	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(ContainerMesh);
	InteractionVolume->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	InteractionVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionVolume->SetGenerateOverlapEvents(true);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(Root);
	StatusLight->SetRelativeLocation(FVector(10.f, 0.f, 60.f));
	StatusLight->SetIntensity(300.f);
	StatusLight->SetAttenuationRadius(100.f);
	StatusLight->SetLightColor(FLinearColor(0.8f, 0.1f, 0.1f)); // red = locked
}

void AZP_ObjectiveContainer::BeginPlay()
{
	Super::BeginPlay();

	// Fit the interaction trigger to whatever mesh this instance uses (+ padding).
	UZP_ContainerUtils::FitBoxToMeshBounds(ContainerMesh, InteractionVolume, InteractionPadding);

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_ObjectiveContainer::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_ObjectiveContainer::OnOverlapEnd);

	// Restore completed state from a save: the objective flag is persisted by the subsystem.
	if (IsObjectiveFlagAlreadySet())
	{
		bUnlocked = true;
		SetStatusLightColor(FLinearColor(0.1f, 0.8f, 0.1f));
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s: restored UNLOCKED from flag '%s'"),
			*GetName(), *ObjectiveFlagOnUnlock.ToString());
		return; // already done — don't auto-lock doors
	}

	// Auto-lock any InteractDoors within DoorLockRadius (mirrors the card reader).
	if (DoorLockRadius > 0.f)
	{
		const FVector MyLocation = GetActorLocation();
		for (TActorIterator<AZP_InteractDoor> It(GetWorld()); It; ++It)
		{
			AZP_InteractDoor* Door = *It;
			if (Door && FVector::Dist(MyLocation, Door->GetActorLocation()) <= DoorLockRadius)
			{
				Door->bLocked = true;
				AutoLockedDoors.Add(Door);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s: Ready — %d required item type(s), Flag=%s, LinkedDoor=%s, AutoLockedDoors=%d"),
		*GetName(), RequiredItems.Num(), *ObjectiveFlagOnUnlock.ToString(),
		LinkedDoor ? *LinkedDoor->GetName() : TEXT("NONE"), AutoLockedDoors.Num());
}

// --- IZP_Interactable ---

FText AZP_ObjectiveContainer::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_ObjectiveContainer::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bUnlocked)
	{
		return;
	}

	if (RequiredItems.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveContainer %s: no RequiredItems configured — cannot unlock"), *GetName());
		return;
	}

	AZP_PlayerController* PC = Interactor ? Cast<AZP_PlayerController>(Interactor->GetController()) : nullptr;

	if (HasAllRequiredItems(Interactor))
	{
		Unlock(Interactor);
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(UnlockedMessage);
		}
	}
	else
	{
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(MissingItemsMessage);
		}
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s: player missing required items"), *GetName());
	}
}

// --- Overlap (interactable registration, mirrors card reader) ---

void AZP_ObjectiveContainer::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace || bUnlocked) return;

	Grace->SetCurrentInteractable(this);

	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(PromptText);
		}
	}
}

void AZP_ObjectiveContainer::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	Grace->ClearCurrentInteractable(this);

	if (AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->HideInteractionPrompt();
		}
	}
}

// --- Inventory (reflection, mirrors card reader / FilterLockerAmmo) ---

UActorComponent* AZP_ObjectiveContainer::GetMoonvilleInventoryComp(ACharacter* Character) const
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Character);
	return Grace ? Grace->MoonvilleInventoryComp : nullptr;
}

int32 AZP_ObjectiveContainer::CountItemInInventory(UActorComponent* InvComp, UObject* TargetDA) const
{
	if (!InvComp || !TargetDA) return 0;

	int32 Total = 0;
	const FName ArrayNames[] = { FName("ItemSlots"), FName("ShortcutSlots") };
	for (const FName& ArrayName : ArrayNames)
	{
		FProperty* SlotsProp = InvComp->GetClass()->FindPropertyByName(ArrayName);
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
		if (!ArrayProp) continue;

		FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(InvComp));
		FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
		if (!StructInner) continue;

		FObjectProperty* ItemProp = nullptr;
		FIntProperty* AmountProp = nullptr;
		for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
		{
			if (!ItemProp && It->GetName().Contains(TEXT("Item_")))   ItemProp = CastField<FObjectProperty>(*It);
			if (!AmountProp && It->GetName().Contains(TEXT("Amount"))) AmountProp = CastField<FIntProperty>(*It);
		}
		if (!ItemProp) continue;

		for (int32 i = 0; i < ArrayHelper.Num(); ++i)
		{
			void* Elem = ArrayHelper.GetRawPtr(i);
			UObject* SlotItem = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Elem));
			if (SlotItem != TargetDA) continue;
			const int32 Amt = AmountProp
				? AmountProp->GetPropertyValue(AmountProp->ContainerPtrToValuePtr<void>(Elem))
				: 1;
			Total += FMath::Max(Amt, 1);
		}
	}
	return Total;
}

bool AZP_ObjectiveContainer::HasAllRequiredItems(ACharacter* Character) const
{
	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);
	if (!InvComp) return false;

	for (const FZP_RequiredItem& Req : RequiredItems)
	{
		UObject* DA = Req.Item.LoadSynchronous();
		if (!DA)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] ObjectiveContainer %s: a RequiredItem failed to load"), *GetName());
			return false;
		}
		const int32 Need = FMath::Max(Req.Count, 1);
		if (CountItemInInventory(InvComp, DA) < Need)
		{
			return false;
		}
	}
	return true;
}

void AZP_ObjectiveContainer::Unlock(ACharacter* Character)
{
	if (bUnlocked) return;

	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);

	if (bConsumeItemsOnUnlock && InvComp)
	{
		UFunction* RemoveFunc = InvComp->FindFunction(FName("RemoveItemByDataAsset"));
		if (RemoveFunc)
		{
			for (const FZP_RequiredItem& Req : RequiredItems)
			{
				UObject* DA = Req.Item.LoadSynchronous();
				if (!DA) continue;
				struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
				Params.ItemDataAsset = DA;
				Params.AmountToRemove = FMath::Max(Req.Count, 1);
				InvComp->ProcessEvent(RemoveFunc, &Params);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s: consumed %s x%d"),
					*GetName(), *DA->GetName(), Params.AmountToRemove);
			}
		}
	}

	bUnlocked = true;

	// Open the linked door + any auto-locked doors.
	if (LinkedDoor)
	{
		LinkedDoor->Unlock();
		LinkedDoor->OpenDoor();
	}
	for (auto& DoorRef : AutoLockedDoors)
	{
		if (DoorRef.IsValid())
		{
			DoorRef->Unlock();
		}
	}

	SetStatusLightColor(FLinearColor(0.1f, 0.8f, 0.1f)); // green

	// Persist + advance objectives (flag also restores unlocked state on load).
	SetObjectiveFlag(ObjectiveFlagOnUnlock);

	// Per-instance custom outcome (power on, SFX, etc.).
	OnUnlocked();

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveContainer %s: UNLOCKED (flag=%s, door=%s)"),
		*GetName(), *ObjectiveFlagOnUnlock.ToString(), LinkedDoor ? *LinkedDoor->GetName() : TEXT("NONE"));
}

void AZP_ObjectiveContainer::SetStatusLightColor(FLinearColor Color)
{
	if (StatusLight)
	{
		StatusLight->SetLightColor(Color);
	}
}

void AZP_ObjectiveContainer::SetObjectiveFlag(FName Flag)
{
	if (Flag.IsNone()) return;
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr)
	{
		Obj->SetFlag(Flag);
	}
}

bool AZP_ObjectiveContainer::IsObjectiveFlagAlreadySet() const
{
	if (ObjectiveFlagOnUnlock.IsNone()) return false;
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr)
	{
		return Obj->HasFlag(ObjectiveFlagOnUnlock);
	}
	return false;
}
