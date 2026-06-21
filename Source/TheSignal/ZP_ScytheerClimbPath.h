// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ScytheerClimbPath
 *
 * Purpose: A USplineComponent-backed actor that defines a route for a Scytheer to walk.
 *          Each spline point carries an associated WallNormal (world-space FVector) that
 *          says which direction is "up" for the Scytheer at that point — i.e. which way
 *          its head points (away from the surface it is clinging to). The normal is lerped
 *          between adjacent points so one path can run up a wall and curve onto the floor.
 *
 *          MVP usage: drop the actor, shape the spline (start on a wall, end on the floor),
 *          edit PerPointWallNormals in the details panel so each point's normal faces
 *          OUT of the surface that point is sitting on. The Scytheer ping-pongs along it.
 *
 * Owner Subsystem: EnemyAI / ScytheerAmbush
 *
 * Blueprint Extension Points: none — pure C++ for the MVP.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_ScytheerClimbPath.generated.h"

class USplineComponent;

UCLASS()
class THESIGNAL_API AZP_ScytheerClimbPath : public AActor
{
	GENERATED_BODY()

public:
	AZP_ScytheerClimbPath();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Path")
	TObjectPtr<USplineComponent> Spline;

	/** One entry per spline point. World-space direction that points AWAY from the surface
	 *  (the Scytheer's head direction) at that point. Defaults to world up (Z+). Auto-resized
	 *  in OnConstruction to match the spline's current point count. Edit per-point in the
	 *  details panel — for a wall pointing +X, the wall normal is +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path|Scytheer")
	TArray<FVector> PerPointWallNormals;

	// ── Editor authoring ─────────────────────────────────────────────
#if WITH_EDITORONLY_DATA
	/** Number of points a single "Add Points To End" click appends. */
	UPROPERTY(EditAnywhere, Category = "Path|Authoring", meta = (ClampMin = "1", ClampMax = "50"))
	int32 PointsPerAdd = 1;

	/** Step (UU) used for the new point(s) when the path has fewer than 2 points
	 *  to extrapolate a direction from. Once there are 2+ points, the spacing of
	 *  the last segment is reused instead. */
	UPROPERTY(EditAnywhere, Category = "Path|Authoring", meta = (ClampMin = "1"))
	float DefaultStep = 200.f;

	/** How far (UU) "Snap Wall Normals To Geometry" probes around each point to find the
	 *  surface that point rests on. Raise it if your points sit a little off the wall/floor. */
	UPROPERTY(EditAnywhere, Category = "Path|Authoring", meta = (ClampMin = "1"))
	float NormalProbeDistance = 100.f;
#endif

#if WITH_EDITOR
	/** Details-panel button: appends PointsPerAdd point(s) to the END of the path,
	 *  continuing the last segment's direction so you can keep extending it while you
	 *  map the route. The new point inherits the last point's wall normal. */
	UFUNCTION(CallInEditor, Category = "Path|Authoring")
	void AddPointsToEnd();

	/** Details-panel button: line-traces around every spline point and sets each point's
	 *  wall normal to the surface it rests on (floor -> up, wall -> out of the wall). Run this
	 *  after shaping the route so the Scytheer orients correctly up walls without hand-typing
	 *  normals. Points that find no nearby surface keep their current normal. */
	UFUNCTION(CallInEditor, Category = "Path|Authoring")
	void SnapWallNormalsToGeometry();
#endif

	// ── Queries used by the Scytheer ─────────────────────────────────
	float GetSplineLength() const;
	FVector GetLocationAtDistance(float Distance) const;
	FVector GetForwardAtDistance(float Distance) const;
	/** Lerped per-point wall normal at this distance, world-space, normalized. */
	FVector GetWallNormalAtDistance(float Distance) const;
	/** Distance-along-spline of the point on the spline closest to WorldPoint. Used by the Scytheer
	 *  to bias travel direction toward the player when aggroed. */
	float GetClosestDistanceToPoint(const FVector& WorldPoint) const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
