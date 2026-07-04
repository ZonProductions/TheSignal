// Copyright The Signal. All Rights Reserved.

#include "ZP_ScytheerClimbPath.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AZP_ScytheerClimbPath::AZP_ScytheerClimbPath()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;
}

void AZP_ScytheerClimbPath::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!Spline) { return; }

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints <= 0) { return; }

	// Resize wall-normal array to match the spline. Pad new entries with world-up so the
	// dev has a sensible default; existing edits are preserved.
	if (AZP_PerPointWallNormals.Num() < NumPoints)
	{
		const int32 ToAdd = NumPoints - AZP_PerPointWallNormals.Num();
		AZP_PerPointWallNormals.Reserve(NumPoints);
		for (int32 i = 0; i < ToAdd; ++i)
		{
			AZP_PerPointWallNormals.Add(FVector::UpVector);
		}
	}
	else if (AZP_PerPointWallNormals.Num() > NumPoints)
	{
		AZP_PerPointWallNormals.SetNum(NumPoints);
	}
}

#if WITH_EDITOR
void AZP_ScytheerClimbPath::AddPointsToEnd()
{
	if (!Spline) { return; }

	const int32 Count = FMath::Max(1, AZP_PointsPerAdd);

	Modify();
	Spline->Modify();

	for (int32 Added = 0; Added < Count; ++Added)
	{
		const int32 Num = Spline->GetNumberOfSplinePoints();

		// Work out where the next point goes: continue the last segment's direction
		// and spacing so the path keeps flowing the way you were heading.
		FVector NewLoc;
		if (Num >= 2)
		{
			const FVector Last = Spline->GetLocationAtSplinePoint(Num - 1, ESplineCoordinateSpace::World);
			const FVector Prev = Spline->GetLocationAtSplinePoint(Num - 2, ESplineCoordinateSpace::World);
			FVector Dir = Last - Prev;
			const float Step = FMath::Max(Dir.Size(), 1.f);
			Dir = Dir.GetSafeNormal(KINDA_SMALL_NUMBER, GetActorForwardVector());
			NewLoc = Last + Dir * Step;
		}
		else if (Num == 1)
		{
			const FVector Last = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			NewLoc = Last + GetActorForwardVector() * FMath::Max(AZP_DefaultStep, 1.f);
		}
		else
		{
			NewLoc = GetActorLocation();
		}

		Spline->AddSplinePoint(NewLoc, ESplineCoordinateSpace::World, /*bUpdateSpline*/ false);

		// Keep the wall-normal array in lockstep — inherit the previous point's normal
		// so extending a wall section keeps its orientation (edit it after if it turns).
		const FVector InheritNormal = AZP_PerPointWallNormals.Num() > 0 ? AZP_PerPointWallNormals.Last() : FVector::UpVector;
		AZP_PerPointWallNormals.Add(InheritNormal);
	}

	Spline->UpdateSpline();

	// Refresh the viewport gizmo + details panel and flag the level dirty for save.
	PostEditChange();
	MarkPackageDirty();
}

void AZP_ScytheerClimbPath::SnapWallNormalsToGeometry()
{
	if (!Spline) { return; }
	UWorld* W = GetWorld();
	if (!W) { return; }

	const int32 Num = Spline->GetNumberOfSplinePoints();
	if (Num <= 0) { return; }
	AZP_PerPointWallNormals.SetNum(Num);

	// Probe the 6 axis directions; the surface a point RESTS ON is whichever blocking
	// static geometry is nearest. Its outward face normal is exactly the "head direction"
	// the Scytheer should use (floor -> +Z, a wall -> out of the wall).
	const FVector Dirs[6] = {
		FVector(0, 0, 1), FVector(0, 0, -1),
		FVector(1, 0, 0), FVector(-1, 0, 0),
		FVector(0, 1, 0), FVector(0, -1, 0)
	};
	const float Probe = FMath::Max(AZP_NormalProbeDistance, 1.f);

	Modify();

	int32 NumSet = 0;
	for (int32 i = 0; i < Num; ++i)
	{
		const FVector P = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);

		FVector BestNormal = FVector::ZeroVector;
		float BestDist = TNumericLimits<float>::Max();

		for (const FVector& D : Dirs)
		{
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ScytheerPathNormalProbe), false, this);
			// Back off 2 UU on the outward side so a point sitting flush on the surface still traces in.
			const FVector Start = P - D * 2.f;
			const FVector End   = P + D * Probe;
			if (W->LineTraceSingleByObjectType(Hit, Start, End,
					FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				const float Dist = (Hit.ImpactPoint - P).Size();
				if (Dist < BestDist)
				{
					BestDist = Dist;
					BestNormal = Hit.ImpactNormal;
				}
			}
		}

		if (!BestNormal.IsNearlyZero())
		{
			AZP_PerPointWallNormals[i] = BestNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
			++NumSet;
		}
		else if (AZP_PerPointWallNormals[i].IsNearlyZero())
		{
			AZP_PerPointWallNormals[i] = FVector::UpVector; // nothing found — sane default
		}
	}

	PostEditChange();
	MarkPackageDirty();

	UE_LOG(LogTemp, Log, TEXT("[Scytheer] %s: snapped %d/%d wall normals to geometry."),
		*GetActorLabel(), NumSet, Num);
}
#endif

float AZP_ScytheerClimbPath::GetSplineLength() const
{
	return Spline ? Spline->GetSplineLength() : 0.f;
}

FVector AZP_ScytheerClimbPath::GetLocationAtDistance(float Distance) const
{
	return Spline
		? Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World)
		: GetActorLocation();
}

FVector AZP_ScytheerClimbPath::GetForwardAtDistance(float Distance) const
{
	return Spline
		? Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World)
		: FVector::ForwardVector;
}

FVector AZP_ScytheerClimbPath::GetWallNormalAtDistance(float Distance) const
{
	if (!Spline || AZP_PerPointWallNormals.Num() == 0)
	{
		return FVector::UpVector;
	}

	const int32 NumPoints = AZP_PerPointWallNormals.Num();
	if (NumPoints == 1)
	{
		return AZP_PerPointWallNormals[0].GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
	}

	const float Total = Spline->GetSplineLength();
	const float D = FMath::Clamp(Distance, 0.f, Total);

	float Prev = 0.f;
	for (int32 i = 0; i < NumPoints - 1; ++i)
	{
		const float Next = Spline->GetDistanceAlongSplineAtSplinePoint(i + 1);
		if (D <= Next)
		{
			const float Seg = FMath::Max(Next - Prev, KINDA_SMALL_NUMBER);
			const float Alpha = (D - Prev) / Seg;
			return FMath::Lerp(AZP_PerPointWallNormals[i], AZP_PerPointWallNormals[i + 1], Alpha)
				.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
		}
		Prev = Next;
	}

	return AZP_PerPointWallNormals.Last().GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
}

float AZP_ScytheerClimbPath::GetClosestDistanceToPoint(const FVector& WorldPoint) const
{
	if (!Spline) { return 0.f; }
	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(WorldPoint);
	return Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}
