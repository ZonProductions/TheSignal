// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * FZP_WallMap
 *
 * Purpose: Level-wide database of climbable surfaces. Scanned once at level start.
 *          Creatures query this to KNOW where walls are, how high they are,
 *          and where to crest — instead of blindly reacting to collisions.
 *
 * Owner Subsystem: EnemyAI
 *
 * Usage:
 *   FZP_WallMap::Build(World, ScanCenter) — call once from first creature BeginPlay
 *   FZP_WallMap::FindNearest(Location, Radius) — find closest wall
 *   FZP_WallMap::FindBestToward(From, Target, Radius) — find wall on path to target
 *   FZP_WallMap::TraceWallTop(World, HitPoint, Normal) — on-demand height check
 *   FZP_WallMap::Clear() — call on level teardown
 */

#include "CoreMinimal.h"

struct FClimbableWall
{
	FVector Location;	// Point on wall surface where it was detected
	FVector Normal;		// Surface normal (points away from wall)
	float TopZ;			// Z where wall ends — creature crests here
	float BottomZ;		// Z where wall starts (approximate)
};

class THESIGNAL_API FZP_WallMap
{
public:
	/** Scan the level for climbable walls. Traces a grid of horizontal rays
	 *  and records every wall-like surface (|Normal.Z| < 0.5).
	 *  For each wall hit, traces upward to determine TopZ. */
	static void Build(UWorld* World, const FVector& ScanCenter);

	/** Find the closest climbable wall within Radius of Location. */
	static const FClimbableWall* FindNearest(const FVector& Location, float Radius);

	/** Find the best wall to climb toward when heading for a target.
	 *  Scores walls by: direction toward target, distance, wall height. */
	static const FClimbableWall* FindBestToward(
		const FVector& CreatureLocation,
		const FVector& TargetLocation,
		float Radius);

	/** On-demand: trace a specific wall surface upward to find its top Z.
	 *  Used by the CMC when it starts climbing a wall that may not be in the map. */
	static float TraceWallTop(UWorld* World, const FVector& WallPoint, const FVector& WallNormal);

	/** Clear all scan data. Call on level teardown / EndPlay. */
	static void Clear();

	static bool IsBuilt() { return bBuilt; }
	static int32 Num() { return Walls.Num(); }

private:
	static TArray<FClimbableWall> Walls;
	static bool bBuilt;
};
