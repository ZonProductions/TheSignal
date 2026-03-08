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

	/** Slot name used for this save. */
	static const FString DefaultSlotName;
};
