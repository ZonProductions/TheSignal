// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * IZP_Revivable
 *
 * Purpose: Implemented by any enemy whose DEAD state can be (a) restored on save-load and (b) undone
 *          ("revived") when the campaign reaches a beat. UZP_DeathSaveComponent drives both through this
 *          interface, so it stays enemy-agnostic: a new enemy becomes persist-/revive-able simply by
 *          implementing this (no save-component edits).
 *
 *          BlueprintNativeEvent, mirroring IZP_Staggerable, so it works whether the enemy is C++ or
 *          Blueprint, actor- or component-based:
 *            - Component-based AI (Shambler) -> implement on the behavior component.
 *            - Actor-based AI (Scytheer)     -> implement on the actor.
 *
 * Owner Subsystem: SaveSystem / EnemyAI
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZP_Revivable.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UZP_Revivable : public UInterface
{
	GENERATED_BODY()
};

class IZP_Revivable
{
	GENERATED_BODY()

public:
	/** Snap the enemy straight into its dead/corpse state — held final death frame, collision off, AI off —
	 *  WITHOUT replaying the death animation or SFX from the start. Called by UZP_DeathSaveComponent on a
	 *  LOAD when the saved state says this enemy was dead, so the corpse is restored in place. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Revive")
	void ApplyDeadStateInstant();

	/** Reset the enemy from a dead/corpse state back to a fully alive, patrolling state: health to max,
	 *  collision + movement + AI re-enabled, locomotion restored. Called by UZP_DeathSaveComponent when the
	 *  enemy's configured AZP_ReviveOnObjective completes. Safe to call only on a currently-dead enemy. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy|Revive")
	void ReviveEnemy();
};
