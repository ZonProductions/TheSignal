// Copyright The Signal. All Rights Reserved.

#include "ZP_WallMap.h"

// --- Static storage ---
TArray<FClimbableWall> FZP_WallMap::Walls;
bool FZP_WallMap::bBuilt = false;

// --- Scan configuration ---

// How far from center to scan in each direction (UU)
static constexpr float ScanExtent = 10000.f;

// Distance between grid scan points (UU). Lower = more detail, more traces.
static constexpr float GridStep = 500.f;

// How far each horizontal trace extends (UU)
static constexpr float TraceLength = 2000.f;

// Heights to scan at (UU, world Z). Multiple heights catch elevated walls.
static constexpr float ScanHeights[] = { 100.f, 400.f, 800.f };
static constexpr int32 NumScanHeights = UE_ARRAY_COUNT(ScanHeights);

// Step size when tracing up a wall to find its top (UU)
static constexpr float WallTopTraceStep = 50.f;

// Max height to trace upward looking for wall top (UU)
static constexpr float WallTopTraceMax = 3000.f;

// Minimum wall height to be worth recording (UU)
static constexpr float MinWallHeight = 80.f;

// --- Build ---

void FZP_WallMap::Build(UWorld* World, const FVector& ScanCenter)
{
	if (bBuilt || !World) return;

	const double StartTime = FPlatformTime::Seconds();
	Walls.Reset();

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	// 4 cardinal directions — catches walls facing any direction
	static const FVector Dirs[] = {
		FVector(1.f, 0.f, 0.f),
		FVector(-1.f, 0.f, 0.f),
		FVector(0.f, 1.f, 0.f),
		FVector(0.f, -1.f, 0.f)
	};

	const float MinX = ScanCenter.X - ScanExtent;
	const float MaxX = ScanCenter.X + ScanExtent;
	const float MinY = ScanCenter.Y - ScanExtent;
	const float MaxY = ScanCenter.Y + ScanExtent;

	for (int32 H = 0; H < NumScanHeights; ++H)
	{
		const float ScanZ = ScanCenter.Z + ScanHeights[H];

		for (float X = MinX; X <= MaxX; X += GridStep)
		{
			for (float Y = MinY; Y <= MaxY; Y += GridStep)
			{
				const FVector Origin(X, Y, ScanZ);

				for (const FVector& Dir : Dirs)
				{
					FHitResult Hit;
					if (World->LineTraceSingleByChannel(Hit, Origin, Origin + Dir * TraceLength,
						ECC_GameTraceChannel1, Params))
					{
						// Wall surface: normal is roughly horizontal
						if (FMath::Abs(Hit.ImpactNormal.Z) < 0.5f)
						{
							const float TopZ = TraceWallTop(World, Hit.ImpactPoint, Hit.ImpactNormal);
							const float WallHeight = TopZ - Hit.ImpactPoint.Z;

							// Skip tiny walls (curbs, trim, etc.)
							if (WallHeight < MinWallHeight)
							{
								continue;
							}

							Walls.Add({
								Hit.ImpactPoint,
								Hit.ImpactNormal,
								TopZ,
								static_cast<float>(Hit.ImpactPoint.Z)
							});
						}
					}
				}
			}
		}
	}

	bBuilt = true;
	const double Duration = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogTemp, Log, TEXT("[WallMap] Scan complete: %d surfaces found in %.3fs (center=%s)"),
		Walls.Num(), Duration, *ScanCenter.ToCompactString());
}

// --- Queries ---

const FClimbableWall* FZP_WallMap::FindNearest(const FVector& Location, float Radius)
{
	const FClimbableWall* Best = nullptr;
	float BestDistSq = Radius * Radius;

	for (const FClimbableWall& W : Walls)
	{
		const float DistSq = FVector::DistSquared2D(Location, W.Location);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = &W;
		}
	}

	return Best;
}

const FClimbableWall* FZP_WallMap::FindBestToward(
	const FVector& CreatureLocation,
	const FVector& TargetLocation,
	float Radius)
{
	const FVector ToTarget = (TargetLocation - CreatureLocation).GetSafeNormal2D();
	const FClimbableWall* Best = nullptr;
	float BestScore = -FLT_MAX;
	const float RadiusSq = Radius * Radius;

	for (const FClimbableWall& W : Walls)
	{
		const float DistSq = FVector::DistSquared2D(CreatureLocation, W.Location);
		if (DistSq > RadiusSq || DistSq < 100.f * 100.f) continue;

		const float Dist = FMath::Sqrt(DistSq);
		const FVector ToWall = (W.Location - CreatureLocation).GetSafeNormal2D();

		// Direction alignment: prefer walls on the path toward the target
		const float DirScore = FVector::DotProduct(ToWall, ToTarget);

		// Distance: prefer closer walls
		const float DistScore = 1.f - (Dist / Radius);

		// Height: prefer taller walls (more useful for stalking/elevation)
		const float HeightScore = FMath::Clamp((W.TopZ - W.BottomZ) / 500.f, 0.f, 1.f);

		const float Score = DirScore * 0.5f + DistScore * 0.3f + HeightScore * 0.2f;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = &W;
		}
	}

	return Best;
}

// --- Wall height trace ---

float FZP_WallMap::TraceWallTop(UWorld* World, const FVector& WallPoint, const FVector& WallNormal)
{
	if (!World) return WallPoint.Z;

	FCollisionQueryParams Params;
	Params.bTraceComplex = false;

	float TopZ = WallPoint.Z;

	// Trace INTO the wall at increasing heights.
	// When the trace stops hitting: the wall has ended at that height.
	for (float DZ = WallTopTraceStep; DZ < WallTopTraceMax; DZ += WallTopTraceStep)
	{
		// Start offset from the wall, trace perpendicular into it
		const FVector Start = WallPoint + WallNormal * 100.f + FVector(0.f, 0.f, DZ);
		const FVector End = Start - WallNormal * 200.f;

		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_GameTraceChannel1, Params))
		{
			// No wall at this height — found the top
			TopZ = WallPoint.Z + DZ;
			break;
		}
	}

	return TopZ;
}

// --- Cleanup ---

void FZP_WallMap::Clear()
{
	Walls.Reset();
	bBuilt = false;
	UE_LOG(LogTemp, Log, TEXT("[WallMap] Cleared"));
}
