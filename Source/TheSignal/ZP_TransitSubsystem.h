// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_TransitSubsystem
 *
 * Purpose: Owns cross-level travel for the Transit system. Stores the pending arrival
 *          tag, performs the OpenLevel, and hands the arrival tag to the next map's
 *          GameMode (AZP_GameMode::ChoosePlayerStart) so the player spawns at the right spot.
 *
 * Owner Subsystem: Transit (this). GameInstance lifetime — survives OpenLevel.
 *
 * Blueprint Extension Points: TravelTo (callable from the transit panel/widget).
 *
 * Dependencies: UGameplayStatics::OpenLevelBySoftObjectPtr. Inventory carry-over is handled
 *               by the existing UZP_BriefcaseSubsystem.
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_TransitSubsystem.generated.h"

class UWorld;

UCLASS()
class THESIGNAL_API UZP_TransitSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Store the arrival tag and open the target level. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void TravelTo(TSoftObjectPtr<UWorld> TargetLevel, FName ArrivalPointTag);

	/** Read + clear the pending arrival tag (called by the GameMode on the new map). */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	FName ConsumePendingArrivalTag();

	/** Read the pending arrival tag without clearing it. */
	UFUNCTION(BlueprintPure, Category = "Transit")
	FName PeekPendingArrivalTag() const { return PendingArrivalTag; }

private:
	FName PendingArrivalTag = NAME_None;
};
