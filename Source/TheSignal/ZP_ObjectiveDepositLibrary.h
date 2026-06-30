// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ObjectiveDepositLibrary
 *
 * Purpose: Blueprint-callable logic for BP_ObjectiveContainer — a reusable Moonville DEPOSIT container
 *          where the designer configures RequiredItems (in Details) and the player drops those items in
 *          and submits. Keeps the requirement/flag logic in C++ while the container itself stays a
 *          Moonville BP (child of BP_ItemContainer_Horror — can't be reparented to C++, SCS-drop dead end).
 *
 * Owner Subsystem: FacilitySystemsManager.
 *
 * What it does (all via Moonville reflection, same idiom as AZP_GraceCharacter::FilterLockerAmmo):
 *   - SetupDepositGrid: resize the container grid to exactly N cells (N = sum of RequiredItems counts).
 *   - ValidateDeposit:  true once the container holds every RequiredItem in at least its Count.
 *   - SubmitDeposit:    validate -> optionally consume the items -> set the objective flag (persists).
 *
 * Dependencies: FZP_RequiredItem (ZP_ObjectiveContainer.h), UZP_ObjectiveSubsystem, Moonville
 *               BP_InventoryComponent (ItemSlots / InventoryConfig / InventorySizeExpansion / RemoveItemByDataAsset).
 */

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZP_ObjectiveContainer.h" // FZP_RequiredItem
#include "ZP_ObjectiveDepositLibrary.generated.h"

UCLASS()
class THESIGNAL_API UZP_ObjectiveDepositLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The Moonville inventory component on a container that owns the ItemSlots array (the deposit grid).
	 *  Container defaults to self when called from the container BP (no Self node needed). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Container"))
	static UActorComponent* FindContainerInventory(AActor* Container);

	/** Total deposit cells needed = sum of RequiredItems[i].Count (min 1). */
	UFUNCTION(BlueprintPure, Category = "TheSignal|ObjectiveDeposit")
	static int32 RequiredCellCount(const TArray<FZP_RequiredItem>& RequiredItems);

	/** Resize the container's grid to exactly N cells in a single row (N = RequiredCellCount).
	 *  Writes InventorySizeExpansion so GetInventorySize() == (N, 1). Call at BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit")
	static void SetupDepositGrid(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems);

	/** Convenience: find the container's deposit inventory and size its grid in one call.
	 *  Container defaults to self — call straight from the container BP's BeginPlay (single node). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Container"))
	static void SetupDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems);

	/** True only if the container currently holds every RequiredItem in at least its Count. */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit")
	static bool ValidateDeposit(UActorComponent* ContainerInventory, const TArray<FZP_RequiredItem>& RequiredItems);

	/** Validate the deposit; on success optionally consume the required items and set ObjectiveFlag
	 *  on the global UZP_ObjectiveSubsystem (which persists). Returns true if it unlocked.
	 *  Container defaults to self when called from the container BP (no Self node needed). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Container"))
	static bool SubmitDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, bool bConsume, FName ObjectiveFlag);
};
