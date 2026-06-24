// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_SaveGame
 *
 * Purpose: Serializable save data for The Signal. Stores player state
 *          that persists between play sessions.
 *
 * Owner Subsystem: SaveSystem
 */

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_SaveGame.generated.h"

UCLASS()
class THESIGNAL_API UZP_SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Player world location at time of save. */
	UPROPERTY(SaveGame)
	FVector PlayerLocation = FVector::ZeroVector;

	/** Player control rotation at time of save. */
	UPROPERTY(SaveGame)
	FRotator PlayerRotation = FRotator::ZeroRotator;

	/** Player health at time of save. */
	UPROPERTY(SaveGame)
	float PlayerHealth = 100.f;

	/** Player max health at time of save. */
	UPROPERTY(SaveGame)
	float PlayerMaxHealth = 100.f;

	/** Name of the level/map at time of save. */
	UPROPERTY(SaveGame)
	FString LevelName;

	/** Dialogue IDs that have already been played (one-shot tracking). */
	UPROPERTY(SaveGame)
	TSet<FName> PlayedDialogueIDs;

	/** Narrative beats triggered by dialogue choices and events. */
	UPROPERTY(SaveGame)
	TSet<FName> TriggeredNarrativeBeats;

	/** All notes/documents the player has collected (permanent). */
	UPROPERTY(SaveGame)
	TArray<FZP_NoteEntry> CollectedNotes;

	// --- Objective progression (UZP_ObjectiveSubsystem) ---

	/** Objectives currently in progress. */
	UPROPERTY(SaveGame)
	TArray<FName> ActiveObjectives;

	/** Main objectives the player has completed. */
	UPROPERTY(SaveGame)
	TArray<FName> CompletedObjectives;

	/** Sub-objectives (steps) the player has completed. */
	UPROPERTY(SaveGame)
	TArray<FName> CompletedSubObjectives;

	/** Hidden sub-objectives that have been revealed (e.g. after trying a locked card reader). */
	UPROPERTY(SaveGame)
	TArray<FName> RevealedSubObjectives;

	/** Per-sub completed-stage count — drives a multi-stage step's current title across save/load. */
	UPROPERTY(SaveGame)
	TMap<FName, int32> SubObjectiveStages;

	/** Progression flags set by triggers/dialogue/events. */
	UPROPERTY(SaveGame)
	TArray<FName> ProgressFlags;

	/** Trigger ids the player has reached (ReachedTrigger requirements). */
	UPROPERTY(SaveGame)
	TArray<FName> ReachedTriggers;

	// --- Moonville inventory persistence (AZP_GraceCharacter) ---

	/** True once an inventory snapshot has been written (distinguishes a saved-but-empty inventory from
	 *  a brand-new game, so a new game still grants the starting weapon). */
	UPROPERTY(SaveGame)
	bool bHasSavedInventory = false;

	/** Moonville item DataAsset paths (one per occupied grid slot), parallel to SavedInventoryAmounts. */
	UPROPERTY(SaveGame)
	TArray<FString> SavedInventoryItemPaths;

	/** Stack amount for each saved item, parallel to SavedInventoryItemPaths. */
	UPROPERTY(SaveGame)
	TArray<int32> SavedInventoryAmounts;

	/** Slot name used for this save. */
	static const FString DefaultSlotName;
};
