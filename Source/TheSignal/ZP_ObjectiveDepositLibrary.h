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

class UUserWidget;

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

	/** BeginPlay setup for an objective deposit container. Sizes the deposit grid, then EITHER
	 *  (a) if ObjectiveFlag is already set (restored from a save) applies the COMPLETE state
	 *  immediately — locks interaction, no status light — OR (b) spawns a dim white status light
	 *  that stays lit until the deposit is complete. Call once from the container BP's BeginPlay.
	 *  Container defaults to self when called from the container BP (no Self node needed). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Container"))
	static void SetupObjectiveContainer(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, FName ObjectiveFlag);

	/** Auto-complete check for a deposit container. Completes ONLY when the container holds EVERY
	 *  RequiredItem in at least its Count (never on a partial deposit). On completion it sets
	 *  ObjectiveFlag (persists), turns off the status light, and permanently disables interaction so
	 *  the container can't be reopened — it's done. Deposited items are left in the locked container
	 *  (no consume; the player never needs to think about them). Returns true the moment it completes;
	 *  a safe no-op once already complete or while items are still missing. Call from OnCloseInventory.
	 *  Container defaults to self when called from the container BP (no Self node needed). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Container"))
	static bool ProcessObjectiveDeposit(AActor* Container, const TArray<FZP_RequiredItem>& RequiredItems, FName ObjectiveFlag);

	/** Reads the container BP's own RequiredItems + ObjectiveFlag (set per-instance in Details) via
	 *  reflection and runs ProcessObjectiveDeposit on it. Lets the player's inventory watchdog
	 *  auto-complete an OPEN deposit container the instant the right items are dropped in, without the
	 *  caller having to supply the arrays. Returns true the moment it completes. */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit")
	static bool TryAutoCompleteObjectiveContainer(AActor* Container);

	/** Populate the container menu from the OPEN container's authored text (per-instance in Details):
	 *  ContainerTitle REPLACES the "Item Container" header label above the transfer slots, and
	 *  ContainerDescription fills the ObjectiveBodyText block directly below the slots (collapsed when
	 *  empty). A plain loot container has no such fields, so the header keeps its default and no
	 *  description shows. Widgets are resolved by name via GetWidgetFromName (no compile-time dep on the
	 *  pack WBP). Call from the container menu's SetupMenu; Menu defaults to self (no Self node needed). */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|ObjectiveDeposit", meta = (DefaultToSelf = "Menu"))
	static void ApplyObjectiveContainerMenu(UUserWidget* Menu, AActor* Container);
};
