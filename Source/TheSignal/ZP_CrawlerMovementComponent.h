// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_CrawlerMovementComponent
 *
 * Purpose: Minimal movement for the wall-ambush crawler. The crawler is PLACED on its wall and only
 *          ever leaves it — there is NO wall-climbing, ceiling, or pathfinding here. Four exclusive
 *          modes, all driven by PhysFlying (which owns ALL velocity AND rotation):
 *
 *            Cling   — frozen on a wall, body aligned so "up" = wall normal (the wall is its floor)
 *            Launch  — ballistic pounce toward a target, ends on impact
 *            Slam    — brief in-place strike, ends on a timer
 *            Ground  — move toward the target on the floor under gravity, facing travel
 *
 * Owner Subsystem: EnemyAI
 *
 * Dependencies: none (deliberately self-contained).
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

	virtual void PhysFlying(float DeltaTime, int32 Iterations) override;

	/** Ground destination — a static point or a tracked actor (the player). */
	FVector MoveTarget = FVector::ZeroVector;
	TWeakObjectPtr<AActor> MoveTargetActor;
	FVector GetEffectiveTarget() const;

	/** Freeze on a wall with body "up" aligned to WallNormal (the wall becomes the floor). */
	void SetWallCling(const FVector& InWallNormal);

	/** Release the wall — ground/launch/gravity take over. */
	void ReleaseWall();

	/** Ballistic pounce toward a world location (releases the wall first). */
	void BeginLaunch(const FVector& TargetLocation);

	/** Brief in-place strike; flags an impact after HoldDuration seconds. */
	void BeginSlam(float HoldDuration);

	bool IsClinging()  const { return bClinging; }
	bool IsLaunching() const { return bLaunching; }
	bool IsSlamming()  const { return bSlamming; }
	bool IsOnGround()  const { return !bClinging && !bLaunching && !bSlamming; }

	/** True once a launch/slam has connected — behavior reads this to deal damage, then clears it. */
	bool HasImpacted() const { return bImpacted; }
	void ClearImpact()       { bImpacted = false; }

	FVector GetWallNormal() const { return WallNormal; }

private:
	void ApplyBodyRotation(float DeltaTime);

	// Exclusive modes (at most one true).
	bool bClinging = false;
	bool bLaunching = false;
	bool bSlamming = false;

	bool bImpacted = false;

	FVector WallNormal = FVector::ZeroVector;
	float SlamTimer = 0.f;
	float SlamHold = 0.f;
	float LaunchTimer = 0.f;
};
