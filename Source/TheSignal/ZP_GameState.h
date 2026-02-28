// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GameState
 *
 * Purpose: Game-wide state visible to all systems. Tracks narrative act,
 *          facility breach level, and global flags.
 *
 * Owner Subsystem: Core
 *
 * Blueprint Extension Points:
 *   - All state properties BlueprintReadWrite for prototype scripting.
 *   - Extend in Blueprint for additional state as needed.
 *
 * Dependencies:
 *   - None
 */

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ZP_GameState.generated.h"

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** Current narrative act (0 = pre-breach, 1 = Act 1, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	int32 CurrentAct = 0;

	/** Facility breach level (0.0 = contained, 1.0 = full breach). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	float FacilityBreachLevel = 0.0f;

	/** Whether containment has been breached (one-way flag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility")
	bool bContainmentBreached = false;
};
