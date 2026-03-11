// Copyright The Signal. All Rights Reserved.

#include "ZP_BriefcaseSubsystem.h"

void UZP_BriefcaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bHasData = false;
	StoredItemSlotsText.Empty();
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] ZP_BriefcaseSubsystem initialized."));
}

UActorComponent* UZP_BriefcaseSubsystem::FindInventoryComponent(AActor* Actor) const
{
	if (!Actor)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] FindInventoryComponent: Actor is null!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] FindInventoryComponent: Searching %s (class: %s). Listing ALL components:"),
		*Actor->GetName(), *Actor->GetClass()->GetName());

	int32 CompCount = 0;
	UActorComponent* FoundComp = nullptr;

	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (Comp)
		{
			FString CompClassName = Comp->GetClass()->GetName();
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG]   Component %d: %s (class: %s)"),
				CompCount, *Comp->GetName(), *CompClassName);

			if (CompClassName.Contains(TEXT("BP_InventoryComponent")))
			{
				FoundComp = Comp;
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG]   >>> MATCH: Found BP_InventoryComponent!"));
			}
			CompCount++;
		}
	}

	if (!FoundComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] FindInventoryComponent: No component with 'BP_InventoryComponent' in class name found among %d components!"), CompCount);
	}

	return FoundComp;
}

FArrayProperty* UZP_BriefcaseSubsystem::FindItemSlotsProperty(UActorComponent* InvComp) const
{
	if (!InvComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] FindItemSlotsProperty: InvComp is null!"));
		return nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] FindItemSlotsProperty: Searching %s (class: %s). Listing ALL array properties:"),
		*InvComp->GetName(), *InvComp->GetClass()->GetName());

	int32 PropCount = 0;
	FArrayProperty* FoundProp = nullptr;

	for (TFieldIterator<FArrayProperty> It(InvComp->GetClass()); It; ++It)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG]   ArrayProp %d: %s"), PropCount, *It->GetName());

		if (!FoundProp && It->GetName() == TEXT("ItemSlots"))
		{
			FoundProp = *It;
			UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG]   >>> EXACT MATCH: ItemSlots"));
		}
		PropCount++;
	}

	// Partial match fallback
	if (!FoundProp)
	{
		for (TFieldIterator<FArrayProperty> It(InvComp->GetClass()); It; ++It)
		{
			if (It->GetName().Contains(TEXT("ItemSlots")))
			{
				FoundProp = *It;
				UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG]   >>> PARTIAL MATCH: %s"), *It->GetName());
				break;
			}
		}
	}

	if (!FoundProp)
	{
		UE_LOG(LogTemp, Error, TEXT("[ZP-BUG] FindItemSlotsProperty: 'ItemSlots' NOT FOUND among %d array properties!"), PropCount);
	}

	return FoundProp;
}

bool UZP_BriefcaseSubsystem::LoadIntoBriefcase(AActor* BriefcaseActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] === LoadIntoBriefcase called === bHasData=%d, StoredLen=%d, Actor=%s"),
		bHasData, StoredItemSlotsText.Len(),
		BriefcaseActor ? *BriefcaseActor->GetName() : TEXT("NULL"));

	if (!bHasData || StoredItemSlotsText.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] No stored data to load — first briefcase opened this session."));
		return false;
	}

	UActorComponent* InvComp = FindInventoryComponent(BriefcaseActor);
	if (!InvComp)
	{
		return false;
	}

	FArrayProperty* ItemSlotsProp = FindItemSlotsProperty(InvComp);
	if (!ItemSlotsProp)
	{
		return false;
	}

	// Import the stored text back into the ItemSlots property
	void* PropAddr = ItemSlotsProp->ContainerPtrToValuePtr<void>(InvComp);
	const TCHAR* Buffer = *StoredItemSlotsText;

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Importing text into ItemSlots. First 200 chars: %.200s"), Buffer);

	ItemSlotsProp->ImportText_Direct(Buffer, PropAddr, InvComp, PPF_None);

	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] LoadIntoBriefcase SUCCEEDED (%d chars imported)."), StoredItemSlotsText.Len());
	return true;
}

void UZP_BriefcaseSubsystem::SaveFromBriefcase(AActor* BriefcaseActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] === SaveFromBriefcase called === Actor=%s"),
		BriefcaseActor ? *BriefcaseActor->GetName() : TEXT("NULL"));

	UActorComponent* InvComp = FindInventoryComponent(BriefcaseActor);
	if (!InvComp)
	{
		return;
	}

	FArrayProperty* ItemSlotsProp = FindItemSlotsProperty(InvComp);
	if (!ItemSlotsProp)
	{
		return;
	}

	// Export ItemSlots to text format
	const void* PropAddr = ItemSlotsProp->ContainerPtrToValuePtr<void>(InvComp);
	StoredItemSlotsText.Empty();
	ItemSlotsProp->ExportText_Direct(StoredItemSlotsText, PropAddr, nullptr, InvComp, PPF_None);

	bHasData = true;
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] SaveFromBriefcase SUCCEEDED (%d chars exported). First 200 chars: %.200s"),
		StoredItemSlotsText.Len(), *StoredItemSlotsText);
}

void UZP_BriefcaseSubsystem::ClearStoredData()
{
	bHasData = false;
	StoredItemSlotsText.Empty();
	UE_LOG(LogTemp, Warning, TEXT("[ZP-BUG] Cleared stored data."));
}
