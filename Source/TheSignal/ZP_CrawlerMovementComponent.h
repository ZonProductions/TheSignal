// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_CrawlerMovementComponent
 *
 * Purpose: CMC for creatures — ground pursuit + slam attack.
 *          Simple, reliable, no dependencies on external plugins.
 *          Uses MOVE_Flying with manual gravity for consistent ground movement.
 *
 * Owner Subsystem: EnemyAI
 */

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ZP_CrawlerMovementComponent.generated.h"

UCLASS()
class THESIGNAL_API UZP_CrawlerMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UZP_CrawlerMovementComponent();

	virtual void PhysFlying(float deltaTime, int32 Iterations) override;

	/** Static move target. Set by behavior component. */
	FVector MoveTarget = FVector::ZeroVector;

	/** Dynamic move target — tracks an actor (player during Hunt). */
	TWeakObjectPtr<AActor> MoveTargetActor;

	/** Returns MoveTargetActor location if valid, otherwise MoveTarget. */
	FVector GetEffectiveTarget() const;

	/** Begin a slam attack — hold in place, then flag impact. */
	void BeginSlam(float RiseHeight, float HoldDuration, float FallSpeed);

	bool IsSlamming() const { return bSlamming; }
	bool HasSlamImpacted() const { return bSlamImpacted; }
	void ClearSlamImpact() { bSlamImpacted = false; }

	/** True when creature is on a wall/ceiling surface. */
	bool IsClimbing() const { return WallContactTimer > 0.f; }

	bool IsLaunching() const { return false; }
	bool IsLaunchOnCooldown() const { return false; }

	bool bFloorProbeActive = true;

	/** Set by behavior: true when creature should engage wall climbing (returning to perch). */
	bool bClimbEnabled = false;

	/** Set when creature reaches ceiling or leaves a surface. Cleared by behavior. */
	bool bJustCrested = false;

	/** Set when creature reaches the ceiling. Cleared by behavior. */
	bool bOnCeiling = false;

private:
	// --- Climb state ---
	float WallContactTimer = 0.f;
	FVector ContactWallNormal = FVector::ZeroVector;

	// --- Slam state ---
	bool bSlamming = false;
	bool bSlamImpacted = false;
	float SlamTimer = 0.f;
	float SlamHoldTime = 0.f;
};
