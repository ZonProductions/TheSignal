// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * ZP_TransitTypes
 *
 * Purpose: Shared data types for the Transit system (elevators / trams / building entries).
 *          One destination = one floor/area the panel can travel to, plus its gating data.
 *
 * Owner Subsystem: Transit (UZP_TransitSubsystem)
 *
 * Notes: Gating fields (RequiredObjectiveId / RequiredKeyItem / etc.) are authored now but
 *        only enforced from M3. M1 lists every destination as available.
 */

#include "CoreMinimal.h"
#include "ZP_TransitTypes.generated.h"

class UWorld;

UENUM(BlueprintType)
enum class EZP_TransitLockStyle : uint8
{
	/** Locked destination is omitted from the menu until it becomes available ("Marcus learns of it"). */
	HiddenUntilKnown UMETA(DisplayName = "Hidden Until Known"),
	/** Locked destination is shown greyed out with LockedReason text. */
	GreyedWithReason UMETA(DisplayName = "Greyed With Reason")
};

/** A single travel destination offered by a transit panel. */
USTRUCT(BlueprintType)
struct FZP_TransitDestination
{
	GENERATED_BODY()

	/** Stable id (e.g. Building1.Floor3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	FName DestinationId = NAME_None;

	/** Label shown in the menu (e.g. "Floor 3 — Labs"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	FText DisplayName;

	/** Map to travel to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	TSoftObjectPtr<UWorld> TargetLevel;

	/** PlayerStart.PlayerStartTag to spawn at on arrival. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	FName ArrivalPointTag = NAME_None;

	// --- Gating (authored now; enforced from M3) ---

	/** Objective that must be complete before this destination is available. None = no gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	FName RequiredObjectiveId = NAME_None;

	/** Objective progression FLAG that must be set before this destination is available. None = no gate.
	 *  Preferred gate for objective STEPS (sub-objective stages have no id): the step sets a flag via its
	 *  EnterFlag, and flags auto-persist — so the floor stays unlocked across save/load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	FName RequiredFlag = NAME_None;

	/** If the player selects/discovers this but isn't ready, start this objective ("Find L5 keycard"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	FName StartObjectiveIfLocked = NAME_None;

	/** Moonville item data asset required to travel (reuses the card-reader inventory check). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	TSoftObjectPtr<UObject> RequiredKeyItem;

	/** Consume the key item on use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	bool bConsumeKey = false;

	/** Shown when the destination is locked (GreyedWithReason style). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	FText LockedReason;

	/** How a locked destination is displayed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Gating")
	EZP_TransitLockStyle LockStyle = EZP_TransitLockStyle::GreyedWithReason;
};

/** Lightweight per-row data handed to the widget for display. */
USTRUCT(BlueprintType)
struct FZP_TransitMenuEntry
{
	GENERATED_BODY()

	/** Index into the panel's Destinations array. */
	UPROPERTY(BlueprintReadOnly, Category = "Transit")
	int32 DestinationIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Transit")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Transit")
	bool bAvailable = true;

	UPROPERTY(BlueprintReadOnly, Category = "Transit")
	FText LockedReason;
};
