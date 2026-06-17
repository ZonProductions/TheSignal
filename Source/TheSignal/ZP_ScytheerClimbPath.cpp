// Copyright The Signal. All Rights Reserved.

#include "ZP_ScytheerClimbPath.h"
#include "Components/SplineComponent.h"

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
	if (PerPointWallNormals.Num() < NumPoints)
	{
		const int32 ToAdd = NumPoints - PerPointWallNormals.Num();
		PerPointWallNormals.Reserve(NumPoints);
		for (int32 i = 0; i < ToAdd; ++i)
		{
			PerPointWallNormals.Add(FVector::UpVector);
		}
	}
	else if (PerPointWallNormals.Num() > NumPoints)
	{
		PerPointWallNormals.SetNum(NumPoints);
	}
}

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
	if (!Spline || PerPointWallNormals.Num() == 0)
	{
		return FVector::UpVector;
	}

	const int32 NumPoints = PerPointWallNormals.Num();
	if (NumPoints == 1)
	{
		return PerPointWallNormals[0].GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
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
			return FMath::Lerp(PerPointWallNormals[i], PerPointWallNormals[i + 1], Alpha)
				.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
		}
		Prev = Next;
	}

	return PerPointWallNormals.Last().GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
}

float AZP_ScytheerClimbPath::GetClosestDistanceToPoint(const FVector& WorldPoint) const
{
	if (!Spline) { return 0.f; }
	const float InputKey = Spline->FindInputKeyClosestToWorldLocation(WorldPoint);
	return Spline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}
