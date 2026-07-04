// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_FloorCullingComponent
 *
 * Purpose: Hides static mesh actors on non-visible floors to reduce rendering
 *          cost in multi-floor buildings. Only renders current floor + adjacent
 *          floors based on player Z position.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - AZP_FloorHeight, AZP_FloorBaseZ, AZP_NumFloors configurable per-level
 *   - AZP_AdjacentFloorsToShow controls neighbor floor visibility
 *
 * Dependencies:
 *   - Must be attached to player character (reads owner location)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_FloorCullingComponent.generated.h"

class UZP_RuntimeISMBatcher;

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_FloorCullingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_FloorCullingComponent();

	/** Height of each floor in UU. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	float AZP_FloorHeight = 500.0f;

	/** Z position of the bottom of Floor 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	float AZP_FloorBaseZ = 0.0f;

	/** Total number of floors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	int32 AZP_NumFloors = 5;

	/** How many floors above and below current to keep visible.
	 *  99 = culling neutralized: single-floor demo maps (Building1_3rdFloor)
	 *  bucket the outdoor night env + Ultra_Dynamic_Sky into floor 0 and hid
	 *  the entire world from floor 3 ("blackbox", session 65). Restore a low
	 *  value only after CollectActors skips sky/env actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	int32 AZP_AdjacentFloorsToShow = 99;

	/** How often to check player floor (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	float AZP_CheckInterval = 0.3f;

	/** Zones where actors are NEVER culled (stairwells, atriums, vertical shafts).
	 *  Actors inside any of these boxes are excluded from both ISM batching and floor culling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	TArray<FBox> AZP_AlwaysVisibleZones;

	/** Class names (Blueprint _C suffix stripped) that are never floor-culled — global/essential actors (sky, fog, post-process, nav). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Culling")
	TArray<FString> AZP_SkipActorClassNames = {
		TEXT("SkyAtmosphere"),
		TEXT("SkyLight"),
		TEXT("DirectionalLight"),
		TEXT("ExponentialHeightFog"),
		TEXT("VolumetricCloud"),
		TEXT("PostProcessVolume"),
		TEXT("LightmassImportanceVolume"),
		TEXT("PlayerStart"),
		TEXT("WorldSettings"),
		TEXT("GameModeBase"),
		TEXT("NavigationData"),
		TEXT("AbstractNavData"),
		TEXT("LevelBounds"),
	};

	/** Current floor the player is on (0-based index). */
	UPROPERTY(BlueprintReadOnly, Category = "Floor Culling")
	int32 CurrentFloor = 0;

	/** ISM batcher — set by GraceCharacter before Initialize(). Used to skip batched actors and toggle ISM visibility. */
	TObjectPtr<UZP_RuntimeISMBatcher> ISMBatcher;

	/** Collects unbatched actors, starts floor check timer. Call after ISMBatcher is wired. */
	void Initialize();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Actors bucketed by floor index. */
	TArray<TArray<TWeakObjectPtr<AActor>>> FloorActors;

	/** Last floor we applied visibility for (avoids redundant updates). */
	int32 LastAppliedFloor = -1;

	FTimerHandle FloorCheckTimerHandle;

	/** Collects all static mesh actors and buckets by floor. */
	void CollectActors();

	/** Timer callback — checks player Z, updates visibility if floor changed. */
	void CheckPlayerFloor();

	/** Shows/hides actors based on target floor. */
	void ApplyFloorVisibility(int32 TargetFloor);

	/** Returns floor index (0-based) for a given Z position. Clamped to valid range. */
	int32 GetFloorForZ(float Z) const;
};
