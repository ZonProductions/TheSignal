// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ObjectiveContainer
 *
 * Purpose: A configurable container/device that ACCEPTS a set of specified items (e.g. "3 fuses")
 *          and, once the player holds them all, consumes them and unlocks — setting an objective
 *          flag, optionally opening a linked door, and firing a BP hook (OnUnlocked) for any
 *          per-instance outcome (power on, play SFX, etc.). Sibling of AZP_CardReaderPanel.
 *
 *          Persistence is free: the unlocked state is derived at BeginPlay from AZP_ObjectiveFlagOnUnlock
 *          (UZP_ObjectiveSubsystem persists flags via the save hook), so a completed container stays
 *          completed across save/load without its own save component.
 *
 * Owner Subsystem: FacilitySystemsManager.
 *
 * Blueprint Extension Points:
 *   - ContainerMesh: set the static mesh in the BP child / per instance.
 *   - AZP_RequiredItems: the item DAs + counts the player must hold to unlock.
 *   - AZP_ObjectiveFlagOnUnlock / AZP_LinkedDoor / OnUnlocked: configure the outcome.
 *
 * Dependencies: IZP_Interactable, Moonville inventory (reflection), UZP_ObjectiveSubsystem,
 *               optionally AZP_LockableDoor / AZP_InteractDoor. Reuses UZP_ContainerUtils.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_ObjectiveContainer.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;
class AZP_LockableDoor;
class AZP_InteractDoor;

/** One item requirement: a Moonville PDA_Item data asset and how many are needed. */
USTRUCT(BlueprintType)
struct FZP_RequiredItem
{
	GENERATED_BODY()

	/** The Moonville PDA_Item data asset required (e.g. DA_Fuse). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ObjectiveContainer")
	TSoftObjectPtr<UObject> Item;

	/** How many of this item the player must hold (e.g. 3 fuses). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ObjectiveContainer")
	int32 Count = 1;
};

UCLASS(Blueprintable)
class THESIGNAL_API AZP_ObjectiveContainer : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_ObjectiveContainer();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Red = locked, green = unlocked. Set intensity 0 to hide for meshes that don't want it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	TObjectPtr<UPointLightComponent> StatusLight;

	// --- Config: requirements ---

	/** Items (and counts) the player must hold to unlock. Empty = never unlockable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	TArray<FZP_RequiredItem> AZP_RequiredItems;

	/** Remove the required items from the player's inventory on unlock. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	bool bAZP_ConsumeItemsOnUnlock = true;

	/** Extra half-extent (UU) added around the mesh bounds for the interaction trigger volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	float AZP_InteractionPadding = 80.f;

	// --- Config: feedback text ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	FText AZP_PromptText = FText::FromString(TEXT("Use"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	FText AZP_MissingItemsMessage = FText::FromString(TEXT("Required items missing"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	FText AZP_UnlockedMessage = FText::FromString(TEXT("Complete"));

	/** Status light color when the container is unlocked (green); hardcoded identically at ZP_ObjectiveContainer.cpp:58 (save-restore path) and ZP_ObjectiveContainer.cpp:270 (Unlock path) — one knob covers both sites. Locked-red counterpart lives as the StatusLight component default (ctor .cpp:41), already editable per-instance on the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ObjectiveContainer|Feedback")
	FLinearColor AZP_UnlockedLightColor = FLinearColor(0.1f, 0.8f, 0.1f);

	// --- Config: outcome ---

	/** Objective flag set when unlocked. Also RESTORES the unlocked state on load (free persistence). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer|Objective")
	FName AZP_ObjectiveFlagOnUnlock = NAME_None;

	/** Optional door unlocked + opened on completion (slide/gate type). */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ObjectiveContainer")
	TObjectPtr<AZP_LockableDoor> AZP_LinkedDoor;

	/** Auto-lock InteractDoors within this radius at BeginPlay, unlocked on completion (0 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveContainer")
	float AZP_DoorLockRadius = 0.f;

	/** Per-instance unlock action — implement in a BP child (power on, play SFX, open a hatch, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "ObjectiveContainer")
	void OnUnlocked();

	UFUNCTION(BlueprintPure, Category = "ObjectiveContainer")
	bool IsUnlocked() const { return bUnlocked; }

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	bool bUnlocked = false;

	/** InteractDoors auto-locked by this container at BeginPlay. */
	TArray<TWeakObjectPtr<AZP_InteractDoor>> AutoLockedDoors;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Moonville inventory component from the character. */
	UActorComponent* GetMoonvilleInventoryComp(ACharacter* Character) const;

	/** How many of TargetDA the player holds across ItemSlots + ShortcutSlots (sums stack Amounts). */
	int32 CountItemInInventory(UActorComponent* InvComp, UObject* TargetDA) const;

	/** True only if the player holds every RequiredItem in at least its required Count. */
	bool HasAllRequiredItems(ACharacter* Character) const;

	/** Consume items (if configured), unlock, open doors, set flag, fire OnUnlocked. */
	void Unlock(ACharacter* Character);

	void SetStatusLightColor(FLinearColor Color);
	void SetObjectiveFlag(FName Flag);

	/** True if AZP_ObjectiveFlagOnUnlock is already set on the subsystem (restored from a save). */
	bool IsObjectiveFlagAlreadySet() const;
};
