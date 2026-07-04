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

	/** Downward gravity acceleration applied to the crawler during launch arcs and ground pursuit (used both for arc solving in BeginLaunch and per-frame fall in PhysFlying). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float AZP_CrawlerGravity = 980.f;

	/** Maximum downward fall speed the crawler can reach while launching or falling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float AZP_TerminalVelocity = 2000.f;

	/** Horizontal speed used to solve the ballistic pounce arc — governs the L4D-Hunter fast/low pounce feel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float AZP_LaunchHorizSpeed = 1400.f;

	/** Lower clamp on the solved pounce flight time — shorter minimum makes close-range pounces snappier and flatter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float AZP_LaunchTimeMin = 0.30f;

	/** Upper clamp on the solved pounce flight time — caps how long/floaty a long-range pounce arc can be. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float AZP_LaunchTimeMax = 0.85f;

	/** Safety timeout in seconds after which an in-flight launch force-ends and flags an impact so the crawler never flies forever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float AZP_LaunchMaxDuration = 1.5f;

	/** VInterpTo ease rate for horizontal velocity while ground-pursuing the target — higher snaps to full chase speed faster. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Ground")
	float AZP_GroundPursuitInterpSpeed = 8.f;

	/** World Z below which a runaway crawler destroys itself — may need per-level tuning on maps with deep geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Safety")
	float AZP_VoidKillZ = -5000.f;

	/** RInterpTo rate for turning the body to face travel direction while on the ground or in the air. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Rotation")
	float AZP_BodyRotationRate = 10.f;

	/** RInterpTo rate for aligning the body to the wall normal while clinging (wall-as-floor alignment speed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Rotation")
	float AZP_ClingRotationRate = 12.f;

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
