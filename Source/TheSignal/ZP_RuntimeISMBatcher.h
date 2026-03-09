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
 *   - MinInstanceCount: minimum instances before batching (default 10)
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
	int32 MinInstanceCount = 10;

	/** Floor params — synced from FloorCullingComponent before BatchStaticMeshes(). */
	float FloorHeight = 500.0f;
	float FloorBaseZ = 0.0f;
	int32 NumFloors = 5;

	/** Zones where actors should NOT be batched (stairwells, etc.). Synced from FloorCullingComponent. */
	TArray<FBox> AlwaysVisibleZones;

	/** Creates per-floor ISMCs from all StaticMeshActors. Call after floor params are set. */
	void BatchStaticMeshes();

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
