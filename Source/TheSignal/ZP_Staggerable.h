// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * IZP_Staggerable
 *
 * Purpose: Implemented by anything the player can stagger — a melee (pipe) hit and a successful
 *          block both call ReceiveStagger() on the target. Lets the player code stay enemy-agnostic:
 *          AZP_GraceCharacter::StaggerEnemy() looks for this interface on the hit actor OR any of its
 *          components, so a new enemy becomes staggerable simply by implementing it (no player edits).
 *
 *          BlueprintNativeEvent, so it works whether the enemy is C++ or Blueprint, actor- or
 *          component-based:
 *            - Component-based AI (Shambler) -> implement on the behavior component.
 *            - Actor-based AI (Scytheer)     -> implement on the actor.
 *
 * Owner Subsystem: EnemyAI
 */

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZP_Staggerable.generated.h"

UINTERFACE(BlueprintType, MinimalAPI)
class UZP_Staggerable : public UInterface
{
	GENERATED_BODY()
};

class IZP_Staggerable
{
	GENERATED_BODY()

public:
	/** Stagger this enemy for Duration seconds: play the hit/flinch reaction and pause its AI.
	 *  Called by the player on a landed melee hit (AZP_HitStaggerDuration) and on a blocked attack
	 *  (AZP_BlockStaggerDuration). Implementations should bypass any internal hit-react cooldown so the
	 *  stagger always reads. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Stagger")
	void ReceiveStagger(float Duration);
};
