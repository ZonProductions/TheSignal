// Copyright The Signal. All Rights Reserved.

#include "ZP_ObjectiveDepositLibrary.h"
#include "ZP_ObjectiveSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	// Find the Item_ (object) and Amount (int) fields inside a Moonville FFItemSlot struct.
	void FindSlotFields(FStructProperty* Inner, FObjectProperty*& OutItem, FIntProperty*& OutAmount)
	{
		OutItem = nullptr; OutAmount = nullptr;
		if (!Inner) return;
		for (TFieldIterator<FProperty> It(Inner->Struct); It; ++It)
		{
			if (!OutItem && It->GetName().Contains(TEXT("Item_")))   OutItem = CastField<FObjectProperty>(*It);
			if (!OutAmount && It->GetName().Contains(TEXT("Amount"))) OutAmount = CastField<FIntProperty>(*It);
		}
	}

	// Sum the stack Amounts of TargetDA across the inventory component's ItemSlots array.
	int32 CountInItemSlots(UActorComponent* Inv, UObject* TargetDA)
	{
		if (!Inv || !TargetDA) return 0;
		FArrayProperty* ArrayProp = CastField<FArrayProperty>(Inv->GetClass()->FindPropertyByName(FName("ItemSlots")));
		if (!ArrayProp) return 0;
		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Inv));
		FObjectProperty* ItemProp = nullptr; FIntProperty* AmountProp = nullptr;
		FindSlotFields(CastField<FStructProperty>(ArrayProp->Inner), ItemProp, AmountProp);
		if (!ItemProp) return 0;

		int32 Total = 0;
		for (int32 i = 0; i < Helper.Num(); ++i)
		{
			void* Elem = Helper.GetRawPtr(i);
			UObject* SlotItem = ItemProp->GetObjectPropertyValue(ItemProp->ContainerPtrToValuePtr<void>(Elem));
			if (SlotItem == TargetDA)
			{
				const int32 Amt = AmountProp ? AmountProp->GetPropertyValue(AmountProp->ContainerPtrToValuePtr<void>(Elem)) : 1;
				Total += FMath::Max(Amt, 1);
			}
		}
		return Total;
	}
}

UActorComponent* UZP_ObjectiveDepositLibrary::FindContainerInventory(AActor* Container)
{
	if (!Container) return nullptr;
	// The deposit grid is the inventory component that owns an ItemSlots array (same probe FilterLockerAmmo uses).
	for (UActorComponent* C : Container->GetComponents())
	{
		if (C && C->GetClass()->FindPropertyByName(FName("ItemSlots")))
		{
			return C;
		}
	}
	return nullptr;
}

int32 UZP_ObjectiveDepositLibrary::RequiredCellCount(const TArray<FZP_RequiredItem>& RequiredItems)
{
	int32 N = 0;
	for (const FZP_RequiredItem& R : RequiredItems)
	{
		N += FMath::Max(R.Count, 1);
	}
	return N;
}

void UZP_ObjectiveDepositLibrary::SetupDepositGrid(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (!ContainerInventory) return;
	const int32 N = FMath::Max(RequiredCellCount(RequiredItems), 1);

	// Base grid size from the config asset (default to 1x1 if unreadable).
	FVector2D Base(1.f, 1.f);
	if (FObjectProperty* CfgProp = CastField<FObjectProperty>(ContainerInventory->GetClass()->FindPropertyByName(FName("InventoryConfig"))))
	{
		if (UObject* Cfg = CfgProp->GetObjectPropertyValue(CfgProp->ContainerPtrToValuePtr<void>(ContainerInventory)))
		{
			if (FStructProperty* SizeProp = CastField<FStructProperty>(Cfg->GetClass()->FindPropertyByName(FName("InventorySize"))))
			{
				Base = *SizeProp->ContainerPtrToValuePtr<FVector2D>(Cfg);
			}
		}
	}

	// GetInventorySize() == InventoryConfig.InventorySize + InventorySizeExpansion. Target one row of N.
	if (FStructProperty* ExpProp = CastField<FStructProperty>(ContainerInventory->GetClass()->FindPropertyByName(FName("InventorySizeExpansion"))))
	{
		FVector2D* Exp = ExpProp->ContainerPtrToValuePtr<FVector2D>(ContainerInventory);
		*Exp = FVector2D(static_cast<float>(N), 1.f) - Base;
	}
}

void UZP_ObjectiveDepositLibrary::SetupDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (UActorComponent* Inv = FindContainerInventory(Container))
	{
		SetupDepositGrid(Inv, RequiredItems);
	}
}

bool UZP_ObjectiveDepositLibrary::ValidateDeposit(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems)
{
	if (!ContainerInventory || RequiredItems.Num() == 0) return false;
	for (const FZP_RequiredItem& R : RequiredItems)
	{
		UObject* DA = R.Item.LoadSynchronous();
		if (!DA) return false;
		if (CountInItemSlots(ContainerInventory, DA) < FMath::Max(R.Count, 1))
		{
			return false;
		}
	}
	return true;
}

bool UZP_ObjectiveDepositLibrary::SubmitDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, bool bConsume, FName ObjectiveFlag)
{
	UActorComponent* Inv = FindContainerInventory(Container);
	if (!Inv || !ValidateDeposit(Inv, RequiredItems))
	{
		return false;
	}

	if (bConsume)
	{
		if (UFunction* RemoveFunc = Inv->FindFunction(FName("RemoveItemByDataAsset")))
		{
			for (const FZP_RequiredItem& R : RequiredItems)
			{
				UObject* DA = R.Item.LoadSynchronous();
				if (!DA) continue;
				struct { UObject* ItemDataAsset; int32 AmountToRemove; } Params;
				Params.ItemDataAsset = DA;
				Params.AmountToRemove = FMath::Max(R.Count, 1); // exact total — Moonville rejects over-removal
				Inv->ProcessEvent(RemoveFunc, &Params);
			}
		}
	}

	if (!ObjectiveFlag.IsNone() && Container)
	{
		UWorld* World = Container->GetWorld();
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		if (UZP_ObjectiveSubsystem* Obj = GI ? GI->GetSubsystem<UZP_ObjectiveSubsystem>() : nullptr)
		{
			Obj->SetFlag(ObjectiveFlag);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] ObjectiveDeposit: SubmitDeposit OK on %s (flag=%s, consumed=%d)"),
		*GetNameSafe(Container), *ObjectiveFlag.ToString(), bConsume ? 1 : 0);
	return true;
}
