// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_CrawlerMovementComponent
 *
 * Purpose: CMC for creatures — owns ALL velocity computation.
 *          Three clean modes: ground pursuit, climbing, launching.
 *          No plugin pathfinding, no CalcVelocity — zero conflicts.
 *
 *          Behavior component sets MoveTarget/MoveTargetActor to direct movement.
 *          LaunchAtTarget() provides ballistic leap (behavior-controlled only).
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points: None — transparent CMC replacement.
 *
 * Dependencies:
 *   - UCharacterMovementComponent (base class)
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

	/** Launch the creature toward a target location in a ballistic arc. */
	void LaunchAtTarget(const FVector& TargetLocation);

	/** Static move target — patrol waypoints, stalk positions, last known player location.
	 *  Used when MoveTargetActor is null. Set by behavior component. */
	FVector MoveTarget = FVector::ZeroVector;

	/** Dynamic move target — tracks an actor (player during Hunt).
	 *  When valid, overrides MoveTarget. Set by behavior component. */
	TWeakObjectPtr<AActor> MoveTargetActor;

	/** True while the creature is in a ballistic arc. */
	bool IsLaunching() const { return bLaunching; }

	/** True if launch is on cooldown (post-landing). */
	bool IsLaunchOnCooldown() const;

	/** Begin a slam attack — rise up, hold at peak, slam down fast. */
	void BeginSlam(float RiseHeight, float HoldDuration, float FallSpeed);

	/** True while any slam phase is active. */
	bool IsSlamming() const { return bSlamming; }

	/** True on the frame slam hits the ground. Cleared by ClearSlamImpact(). */
	bool HasSlamImpacted() const { return bSlamImpacted; }

	/** Clear the slam impact flag after the behavior component handles it. */
	void ClearSlamImpact() { bSlamImpacted = false; }

	/** True when the creature is climbing a wall (wall contact active). */
	bool IsClimbing() const { return WallContactTimer > 0.f; }

	/** When true, ground pursuit checks for dropoffs ahead and stops.
	 *  Set by behavior component — enabled during Patrol, disabled during Hunt/Stalk. */
	bool bFloorProbeActive = true;

	/** When true, creature is allowed to engage wall climbing on contact.
	 *  Set by behavior component — false during Patrol (ground wander),
	 *  true when intentionally seeking walls (Hunt, Stalk, wall-seek patrol).
	 *  Prevents "water bug" bouncing from auto-climbing every wall touched. */
	bool bClimbEnabled = false;

	/** Set true on any crest (ceiling/floor/target-Z). Cleared by behavior component.
	 *  Signals behavior to pause briefly at fence/wall top before resuming pursuit. */
	bool bJustCrested = false;

private:
	/** Returns MoveTargetActor location if valid, otherwise MoveTarget. */
	FVector GetEffectiveTarget() const;

	// --- Climb state ---

	/** Time remaining (seconds) of wall contact. Refreshed on each wall hit, ticks down with deltaTime.
	 *  Keeps climb redirect active through brief surface gaps at edges. */
	float WallContactTimer = 0.f;

	/** Normal of the wall we're climbing — used to compute upward redirect direction. */
	FVector ContactWallNormal = FVector::ZeroVector;

	/** Z position the creature must reach before cresting. Determined by TraceWallTop()
	 *  when climbing first engages. The creature climbs TO this height, then crests —
	 *  no guessing based on hit normals. */
	float ClimbTargetZ = 0.f;

	/** Z position when climb started — used to scale crest push proportionally. */
	float ClimbStartZ = 0.f;

	/** Time spent climbing with no significant height gain. If > threshold, bail. */
	float ClimbStallTime = 0.f;

	/** Z position at last stall check — used to detect recent progress vs total climb. */
	float ClimbLastProgressZ = 0.f;

	/** Cooldown after climb bail-out — prevents re-engaging climb on the same obstacle.
	 *  Ticks down each frame. Climb engagement blocked while > 0. */
	float ClimbBailCooldown = 0.f;

	/** Normal of the last wall we crested — prevents immediately re-climbing the same wall.
	 *  After crest, any wall with dot(normal, LastCrestNormal) > 0.7 is rejected until timer expires. */
	FVector LastCrestNormal = FVector::ZeroVector;

	/** Time remaining before LastCrestNormal expires. While > 0, same-wall re-climb is blocked. */
	float CrestAntiRepeatTimer = 0.f;

	// --- Launch state ---

	/** True during ballistic leap toward player after cresting a wall. */
	bool bLaunching = false;

	/** Current launch velocity — horizontal stays constant, Z decays with gravity. */
	FVector LaunchVelocity = FVector::ZeroVector;

	/** Time remaining before launch auto-expires (safety cap). */
	float LaunchTimeRemaining = 0.f;

	/** Timestamp of last landing — used for cooldown. */
	double LastLandingTime = 0.0;

	/** Z position when launch started — cancel if creature drops too far below. */
	float LaunchStartZ = 0.f;

	// --- Slam state ---
	bool bSlamming = false;
	bool bSlamImpacted = false;
	enum class ESlamPhase : uint8 { Rising, Holding, Falling };
	ESlamPhase SlamPhase = ESlamPhase::Rising;
	float SlamTimer = 0.f;
	float SlamRiseTarget = 0.f;
	float SlamHoldTime = 0.f;
	float SlamDropSpeed = 0.f;
};
