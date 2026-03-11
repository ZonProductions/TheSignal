// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_BriefcaseSubsystem
 *
 * Purpose: Canonical storage for the player's briefcase inventory.
 *          All physical briefcase actors in any level sync to/from this
 *          subsystem so they share the same inventory at runtime.
 *          Persists across level loads (GameInstance lifetime).
 *
 * Owner Subsystem: InventorySystem
 *
 * How it works:
 *   - When a briefcase is opened: LoadIntoBriefcase() deserializes
 *     stored ItemSlots into the briefcase's BP_InventoryComponent.
 *   - When a briefcase is closed: SaveFromBriefcase() serializes
 *     the briefcase's BP_InventoryComponent ItemSlots into storage.
 *   - Storage is a FString (UE text export format) of the ItemSlots TArray.
 *
 * Dependencies:
 *   - Moonville BP_InventoryComponent (Blueprint class, accessed via reflection)
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_BriefcaseSubsystem.generated.h"

UCLASS()
class THESIGNAL_API UZP_BriefcaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Copies stored briefcase inventory data INTO a briefcase actor's
	 * BP_InventoryComponent. Call before the container UI opens.
	 * @param BriefcaseActor The BP_Briefcase actor being opened.
	 * @return true if data was loaded, false if no stored data exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Briefcase")
	bool LoadIntoBriefcase(AActor* BriefcaseActor);

	/**
	 * Copies a briefcase actor's BP_InventoryComponent ItemSlots
	 * into subsystem storage. Call when the container UI closes.
	 * @param BriefcaseActor The BP_Briefcase actor being closed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Briefcase")
	void SaveFromBriefcase(AActor* BriefcaseActor);

	/** Whether we have stored briefcase inventory data. */
	UFUNCTION(BlueprintCallable, Category = "Briefcase")
	bool HasStoredData() const { return bHasData; }

	/** Clears stored data (call on save load to let Moonville's save system take over). */
	UFUNCTION(BlueprintCallable, Category = "Briefcase")
	void ClearStoredData();

private:
	/** Finds the BP_InventoryComponent on an actor by class name. */
	UActorComponent* FindInventoryComponent(AActor* Actor) const;

	/** Finds the ItemSlots FArrayProperty on an inventory component. */
	FArrayProperty* FindItemSlotsProperty(UActorComponent* InvComp) const;

	/** Exported text representation of ItemSlots TArray. */
	FString StoredItemSlotsText;

	/** Whether we have valid stored data. */
	bool bHasData = false;
};
