// Copyright The Signal. All Rights Reserved.

#include "ZP_CardReaderPanel.h"
#include "ZP_LockableDoor.h"
#include "ZP_InteractDoor.h"
#include "EngineUtils.h"
#include "ZP_GraceCharacter.h"
#include "ZP_PlayerController.h"
#include "ZP_HUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Character.h"

AZP_CardReaderPanel::AZP_CardReaderPanel()
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

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(Root);
	StatusLight->SetRelativeLocation(FVector(10.f, 0.f, 60.f));
	StatusLight->SetIntensity(500.f);
	StatusLight->SetAttenuationRadius(100.f);
	StatusLight->SetLightColor(FLinearColor(0.8f, 0.1f, 0.1f)); // red = locked
}

void AZP_CardReaderPanel::BeginPlay()
{
	Super::BeginPlay();

	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AZP_CardReaderPanel::OnOverlapBegin);
	InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AZP_CardReaderPanel::OnOverlapEnd);

	// Auto-lock any InteractDoors within DoorLockRadius
	FVector MyLocation = GetActorLocation();
	for (TActorIterator<AZP_InteractDoor> It(GetWorld()); It; ++It)
	{
		AZP_InteractDoor* Door = *It;
		if (FVector::Dist(MyLocation, Door->GetActorLocation()) <= DoorLockRadius)
		{
			Door->bLocked = true;
			AutoLockedDoors.Add(Door);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Auto-locked door %s (dist=%.0f)"),
				*GetName(), *Door->GetName(), FVector::Dist(MyLocation, Door->GetActorLocation()));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Ready — RequiredItem=%s, LinkedDoor=%s, AutoLockedDoors=%d"),
		*GetName(),
		*RequiredItemName.ToString(),
		LinkedDoor ? *LinkedDoor->GetName() : TEXT("NONE"),
		AutoLockedDoors.Num());
}

// --- IZP_Interactable ---

FText AZP_CardReaderPanel::GetInteractionPrompt_Implementation()
{
	return PromptText;
}

void AZP_CardReaderPanel::OnInteract_Implementation(ACharacter* Interactor)
{
	if (bUnlocked)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Already unlocked — ignoring"),
			*GetName());
		return;
	}

	// Auto-check inventory and use key if present
	if (CheckPlayerHasItem(Interactor))
	{
		UseKey(Interactor);

		// Show access granted via HUD prompt
		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Interactor->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(AccessGrantedMessage);
		}
	}
	else
	{
		// Show missing item message via HUD prompt
		FText Message = FText::Format(MissingItemMessage, RequiredItemName);

		AZP_PlayerController* PC = Cast<AZP_PlayerController>(Interactor->GetController());
		if (PC && PC->HUDWidget)
		{
			PC->HUDWidget->ShowInteractionPrompt(Message);
		}

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Player missing required item: %s"),
			*GetName(), *RequiredItemName.ToString());
	}
}

// --- Overlap (SavePoint pattern) ---

void AZP_CardReaderPanel::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(OtherActor);
	if (!Grace) return;

	// Don't register as interactable once unlocked — let the door handle E-press
	if (bUnlocked) return;

	Grace->SetCurrentInteractable(this);

	AZP_PlayerController* PC = Cast<AZP_PlayerController>(Grace->GetController());
	if (PC && PC->HUDWidget)
	{
		PC->HUDWidget->ShowInteractionPrompt(PromptText);
	}
}

void AZP_CardReaderPanel::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
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

// --- Inventory Access ---

UActorComponent* AZP_CardReaderPanel::GetMoonvilleInventoryComp(ACharacter* Character)
{
	AZP_GraceCharacter* Grace = Cast<AZP_GraceCharacter>(Character);
	if (Grace)
	{
		return Grace->MoonvilleInventoryComp;
	}
	return nullptr;
}

bool AZP_CardReaderPanel::HasItemInSlotArray(UActorComponent* InvComp, const FName& ArrayName, UObject* TargetDA)
{
	if (!InvComp || !TargetDA) return false;

	FProperty* SlotsProp = InvComp->GetClass()->FindPropertyByName(ArrayName);
	if (!SlotsProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: Array '%s' NOT FOUND on %s"),
			*ArrayName.ToString(), *InvComp->GetClass()->GetName());
		return false;
	}

	FArrayProperty* ArrayProp = CastField<FArrayProperty>(SlotsProp);
	if (!ArrayProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: '%s' is not an array property"),
			*ArrayName.ToString());
		return false;
	}

	FScriptArrayHelper ArrayHelper(ArrayProp, SlotsProp->ContainerPtrToValuePtr<void>(InvComp));

	FStructProperty* StructInner = CastField<FStructProperty>(ArrayProp->Inner);
	if (!StructInner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: '%s' inner is not a struct"),
			*ArrayName.ToString());
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel: Searching %s (%d slots), struct=%s"),
		*ArrayName.ToString(), ArrayHelper.Num(), *StructInner->Struct->GetName());

	// Log all struct fields for debugging
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal]   Struct field: '%s' (%s)"),
			*It->GetName(), *It->GetClass()->GetName());
	}

	// Find the Item_ property inside the struct (Moonville slot struct convention)
	FProperty* ItemProp = nullptr;
	for (TFieldIterator<FProperty> It(StructInner->Struct); It; ++It)
	{
		if (It->GetName().Contains(TEXT("Item_")))
		{
			ItemProp = *It;
			break;
		}
	}
	if (!ItemProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: No 'Item_' field found in struct"));
		return false;
	}

	FObjectProperty* ObjProp = CastField<FObjectProperty>(ItemProp);
	if (!ObjProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: '%s' is not an ObjectProperty (is %s)"),
			*ItemProp->GetName(), *ItemProp->GetClass()->GetName());
		return false;
	}

	// Iterate all slots looking for the target DA
	for (int32 i = 0; i < ArrayHelper.Num(); i++)
	{
		void* ElementData = ArrayHelper.GetRawPtr(i);
		UObject* SlotItem = ObjProp->GetObjectPropertyValue(ObjProp->ContainerPtrToValuePtr<void>(ElementData));
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel: %s[%d] item=%s (target=%s)"),
			*ArrayName.ToString(), i,
			SlotItem ? *SlotItem->GetName() : TEXT("NULL"),
			*TargetDA->GetName());
		if (SlotItem == TargetDA)
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel: MATCH in %s[%d]"),
				*ArrayName.ToString(), i);
			return true;
		}
	}

	return false;
}

bool AZP_CardReaderPanel::CheckPlayerHasItem(ACharacter* Character)
{
	UObject* TargetDA = RequiredItemDA.LoadSynchronous();
	if (!TargetDA)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel %s: RequiredItemDA failed to load!"),
			*GetName());
		return false;
	}

	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);
	if (!InvComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel: No MoonvilleInventoryComp on character"));
		return false;
	}

	// Check both ItemSlots (main inventory) and ShortcutSlots
	if (HasItemInSlotArray(InvComp, FName("ItemSlots"), TargetDA))
		return true;
	if (HasItemInSlotArray(InvComp, FName("ShortcutSlots"), TargetDA))
		return true;

	return false;
}

void AZP_CardReaderPanel::UseKey(ACharacter* Character)
{
	if (bUnlocked) return;

	UObject* TargetDA = RequiredItemDA.LoadSynchronous();
	UActorComponent* InvComp = GetMoonvilleInventoryComp(Character);

	// Remove item from inventory (if configured)
	if (bConsumeKeyOnUse && InvComp && TargetDA)
	{
		UFunction* RemoveFunc = InvComp->FindFunction(FName("RemoveItemByDataAsset"));
		if (RemoveFunc)
		{
			struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
			Params.ItemDataAsset = TargetDA;
			Params.AmountToRemove = 1;
			InvComp->ProcessEvent(RemoveFunc, &Params);
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel: Consumed key item %s"),
				*TargetDA->GetName());
		}
	}

	// Unlock and open the linked door
	bUnlocked = true;
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: UseKey — LinkedDoor=%s, bUnlocked=%d"),
		*GetName(),
		LinkedDoor ? *LinkedDoor->GetName() : TEXT("NULL"),
		bUnlocked);
	if (LinkedDoor)
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Calling Unlock+OpenDoor on %s (state=%s)"),
			*GetName(), *LinkedDoor->GetName(),
			*UEnum::GetValueAsString(LinkedDoor->GetDoorState()));
		LinkedDoor->Unlock();
		LinkedDoor->OpenDoor();
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: Door %s unlocked + opened (state=%s)"),
			*GetName(), *LinkedDoor->GetName(),
			*UEnum::GetValueAsString(LinkedDoor->GetDoorState()));
	}

	// Unlock all auto-locked InteractDoors
	for (auto& DoorRef : AutoLockedDoors)
	{
		if (DoorRef.IsValid())
		{
			DoorRef->Unlock();
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] CardReaderPanel %s: InteractDoor %s UNLOCKED"),
				*GetName(), *DoorRef->GetName());
		}
	}

	if (!LinkedDoor && AutoLockedDoors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CardReaderPanel %s: No linked door at UseKey time!"),
			*GetName());
	}

	// Switch status light to green
	SetStatusLightColor(FLinearColor(0.1f, 0.8f, 0.1f));
}

void AZP_CardReaderPanel::SetStatusLightColor(FLinearColor Color)
{
	if (StatusLight)
	{
		StatusLight->SetLightColor(Color);
	}
}
