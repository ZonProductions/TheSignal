// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_RuntimeISMBatcher
 *
 * Purpose: At runtime, scans all StaticMeshActors, groups by (floor, mesh, materials),
 *          creates per-floor InstancedStaticMeshComponents, and hides originals.
 *          Editor keeps individual actors for editing. Runtime gets batched draws.
 *          Integrates with FloorCullingComponent via SetFloorVisible().
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - AZP_MinInstanceCount: minimum instances before batching (default 10)
 *
 * Dependencies:
 *   - Must be attached to a persistent actor (e.g., player character)
 *   - GraceCharacter calls BatchStaticMeshes() after syncing floor params
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ZP_RuntimeISMBatcher.generated.h"

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_RuntimeISMBatcher : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_RuntimeISMBatcher();

	/** Only batch mesh types with at least this many instances (per floor+material group). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ISM Batcher")
	int32 AZP_MinInstanceCount = 10;

	/** PIE/game-only light pass: flip placed MOVABLE shadow-casting lights to STATIONARY at start so
	 *  VSM/Lumen can cache their shadows instead of re-rendering them every frame. A light qualifies
	 *  only when its owning actor is NOT attached to another actor (elevator/door-mounted lights keep
	 *  Movable and keep moving) and is not a Pawn (flashlight, enemies). Runtime-only — the level on
	 *  disk is never modified. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ISM Batcher|Lights")
	bool bAZP_StabilizeLights = true;

	/** Floor params — synced from FloorCullingComponent before BatchStaticMeshes(). */
	float AZP_FloorHeight = 500.0f;
	float AZP_FloorBaseZ = 0.0f;
	int32 AZP_NumFloors = 5;

	/** Zones where actors should NOT be batched (stairwells, etc.). Synced from FloorCullingComponent. */
	TArray<FBox> AZP_AlwaysVisibleZones;

	/** Creates per-floor ISMCs from all StaticMeshActors. Call after floor params are set. */
	void BatchStaticMeshes();

	/** Runtime light-mobility pass (see bAZP_StabilizeLights). Call once at game start. */
	UFUNCTION(BlueprintCallable, Category = "ISM Batcher")
	void StabilizeLights();

	/** Show/hide all ISMCs on a specific floor. Called by FloorCullingComponent. */
	void SetFloorVisible(int32 FloorIndex, bool bVisible);

	/** Returns set of actors that were batched (hidden originals). Floor culling skips these. */
	const TSet<TWeakObjectPtr<AActor>>& GetBatchedActors() const { return BatchedActors; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** ISMCs bucketed by floor index. */
	TArray<TArray<TObjectPtr<UInstancedStaticMeshComponent>>> FloorISMCs;

	/** All original actors that were batched and hidden. */
	TSet<TWeakObjectPtr<AActor>> BatchedActors;

	/** Returns floor index (0-based) for a given Z position. Clamped to valid range. */
	int32 GetFloorForZ(float Z) const;
};
