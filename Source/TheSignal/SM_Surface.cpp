// Copyright The Signal. All Rights Reserved.

#include "SM_Surface.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASM_Surface::ASM_Surface()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// legacy root: never renders or collides (see header — Lumen/PMC + CMC landing)
	SurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceMesh"));
	SetRootComponent(SurfaceMesh);
	SurfaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurfaceMesh->SetCastShadow(false);

	TileISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileISM"));
	TileISM->SetupAttachment(SurfaceMesh);
	TileISM->SetCollisionProfileName(TEXT("BlockAll"));
	TileISM->SetGenerateOverlapEvents(false);
	TileISM->SetCastShadow(true);

	SlabISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SlabISM"));
	SlabISM->SetupAttachment(SurfaceMesh);
	SlabISM->SetCollisionProfileName(TEXT("BlockAll"));
	SlabISM->SetGenerateOverlapEvents(false);
	SlabISM->SetCastShadow(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		SlabISM->SetStaticMesh(CubeFinder.Object);
	}
}

void ASM_Surface::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ASM_Surface::BeginPlay()
{
	Super::BeginPlay();
	Rebuild();
}

void ASM_Surface::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FollowAccum += DeltaSeconds;
	if (FollowAccum < AZP_FollowInterval) { return; }
	FollowAccum = 0.f;
	const uint32 Hash = ComputeStateHash();
	if (Hash != LastStateHash)
	{
		Rebuild();
	}
}

#if WITH_EDITOR
void ASM_Surface::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Rebuild();
}

void ASM_Surface::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	Rebuild();
}
#endif

uint32 ASM_Surface::ComputeStateHash() const
{
	uint32 Hash = GetTypeHash(AZP_CutActors.Num());
	for (const AActor* Cut : AZP_CutActors)
	{
		if (!IsValid(Cut) || Cut == this) { continue; }
		const FBox B = Cut->GetComponentsBoundingBox(/*bNonColliding*/ true);
		// quantize to cm so float noise doesn't retrigger rebuilds
		const FIntVector Mn(B.Min);
		const FIntVector Mx(B.Max);
		Hash = HashCombine(Hash, GetTypeHash(Cut));
		Hash = HashCombine(Hash, GetTypeHash(Mn));
		Hash = HashCombine(Hash, GetTypeHash(Mx));
	}
	// designer-editable state: any change = rebuild on the next follow tick,
	// even when no PostEditChangeProperty fired (e.g. direct component edits)
	Hash = HashCombine(Hash, GetTypeHash(TileISM ? TileISM->GetStaticMesh() : nullptr));
	Hash = HashCombine(Hash, GetTypeHash(FIntVector(static_cast<int32>(AZP_SurfaceSize.X), static_cast<int32>(AZP_SurfaceSize.Y), static_cast<int32>(AZP_Thickness))));
	Hash = HashCombine(Hash, GetTypeHash(FIntVector(static_cast<int32>(AZP_TileSize.X), static_cast<int32>(AZP_TileSize.Y), static_cast<int32>(AZP_CutMargin))));
	Hash = HashCombine(Hash, GetTypeHash(bAZP_GenerateCollision ? 1 : 0));
	return Hash;
}

TArray<FBox2D> ASM_Surface::GatherCutRects() const
{
	TArray<FBox2D> Out;
	const float HX = AZP_SurfaceSize.X * 0.5f;
	const float HY = AZP_SurfaceSize.Y * 0.5f;
	const FTransform MyT = GetActorTransform();
	for (const AActor* Cut : AZP_CutActors)
	{
		if (!IsValid(Cut) || Cut == this) { continue; }
		const FBox WB = Cut->GetComponentsBoundingBox(/*bNonColliding*/ true);
		if (!WB.IsValid) { continue; }
		// world AABB corners -> local space -> local XY AABB (handles our rotation)
		FBox2D L(ForceInit);
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector W(
				(Corner & 1) ? WB.Max.X : WB.Min.X,
				(Corner & 2) ? WB.Max.Y : WB.Min.Y,
				(Corner & 4) ? WB.Max.Z : WB.Min.Z);
			const FVector Local = MyT.InverseTransformPosition(W);
			L += FVector2D(Local.X, Local.Y);
		}
		// negative margins clamp per-rect so a thin piece (stair rail) can
		// never invert and discontinuously drop out of the cut set
		const float HalfMin = 0.5f * FMath::Min(
			static_cast<float>(L.Max.X - L.Min.X), static_cast<float>(L.Max.Y - L.Min.Y));
		L = L.ExpandBy(FMath::Max(AZP_CutMargin, -(HalfMin - 1.f)));
		// clip to the slab footprint
		FBox2D R(ForceInit);
		R.Min = FVector2D(FMath::Max(L.Min.X, -HX), FMath::Max(L.Min.Y, -HY));
		R.Max = FVector2D(FMath::Min(L.Max.X, HX), FMath::Min(L.Max.Y, HY));
		R.bIsValid = true;
		if (R.Max.X - R.Min.X > 1.f && R.Max.Y - R.Min.Y > 1.f)
		{
			Out.Add(R);
		}
	}
	return Out;
}

void ASM_Surface::Rebuild()
{
	if (!SurfaceMesh || !TileISM || !SlabISM) { return; }
	const TArray<FBox2D> Cuts = GatherCutRects();
	LastStateHash = ComputeStateHash();

	SurfaceMesh->ClearAllMeshSections(); // legacy PMC stays empty forever

	const ECollisionEnabled::Type Col = bAZP_GenerateCollision
		? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
	TileISM->SetCollisionEnabled(Col);
	SlabISM->SetCollisionEnabled(Col);

	// The designer owns TileISM's Static Mesh — we only READ it. Mesh set =
	// mesh-tile mode; mesh cleared = cube slab tiles.
	if (TileISM->GetStaticMesh())
	{
		SlabISM->ClearInstances();
		BuildMeshTiles(Cuts);
	}
	else
	{
		TileISM->ClearInstances();
		BuildSlabTiles(Cuts);
	}
}

void ASM_Surface::BuildMeshTiles(const TArray<FBox2D>& Cuts)
{
	TileISM->ClearInstances();

	const FBox MB = TileISM->GetStaticMesh()->GetBoundingBox();
	const float PX = FMath::Max(static_cast<float>(MB.GetSize().X), 1.f);
	const float PY = FMath::Max(static_cast<float>(MB.GetSize().Y), 1.f);

	// NO-STRETCH RULE: every tile is the mesh at its NATURAL size — instances
	// are never scaled. The grid always covers the FULL AZP_SurfaceSize
	// rectangle (swapping meshes changes the look, never the shape): the count
	// rounds UP (small epsilon so exact multiples don't gain a row) and the
	// partial row/column overhangs the edge, split evenly by the centered grid.
	// Cut actors knock out whole tiles.
	const int32 NX = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(AZP_SurfaceSize.X) / PX - 0.01f));
	const int32 NY = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(AZP_SurfaceSize.Y) / PY - 0.01f));
	const float GridX0 = -0.5f * PX * NX; // grid centered on the actor
	const float GridY0 = -0.5f * PY * NY;

	for (int32 I = 0; I < NX; ++I)
	{
		for (int32 J = 0; J < NY; ++J)
		{
			const float X0 = GridX0 + I * PX;
			const float Y0 = GridY0 + J * PY;

			// whole-tile knockout: any real overlap with a cut removes the tile
			// (1uu tolerance so a cut edge flush with the grid line doesn't eat
			// the neighboring row)
			bool bCut = false;
			for (const FBox2D& C : Cuts)
			{
				if (X0 + PX > C.Min.X + 1.f && X0 < C.Max.X - 1.f &&
					Y0 + PY > C.Min.Y + 1.f && Y0 < C.Max.Y - 1.f)
				{
					bCut = true;
					break;
				}
			}
			if (bCut) { continue; }

			// map the mesh box min corner to the cell min corner; mesh TOP at actor Z
			const FVector Pos(X0 - MB.Min.X, Y0 - MB.Min.Y, -MB.Max.Z);
			TileISM->AddInstance(FTransform(FRotator::ZeroRotator, Pos, FVector::OneVector), /*bWorldSpace*/ false);
		}
	}

}

void ASM_Surface::BuildSlabTiles(const TArray<FBox2D>& Cuts)
{
	SlabISM->ClearInstances();
	if (!SlabISM->GetStaticMesh()) { return; }

	// engine cube (100uu, centered pivot, 0-1 UVs per face): one cube per tile
	// = one clean material pattern repeat on the top face.
	const FBox MB = SlabISM->GetStaticMesh()->GetBoundingBox();
	const float CubeX = FMath::Max(static_cast<float>(MB.GetSize().X), 1.f);
	const float CubeY = FMath::Max(static_cast<float>(MB.GetSize().Y), 1.f);
	const float CubeZ = FMath::Max(static_cast<float>(MB.GetSize().Z), 1.f);

	const float PX = FMath::Max(static_cast<float>(AZP_TileSize.X), 1.f);
	const float PY = FMath::Max(static_cast<float>(AZP_TileSize.Y), 1.f);
	const float TZ = FMath::Max(AZP_Thickness, 1.f);

	// same rules as mesh-tile mode: identical tiles, full-rectangle coverage
	// (count rounds UP, centered grid, overhang instead of pattern scaling),
	// whole-tile cut knockout.
	const int32 NX = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(AZP_SurfaceSize.X) / PX - 0.01f));
	const int32 NY = FMath::Max(1, FMath::CeilToInt32(static_cast<float>(AZP_SurfaceSize.Y) / PY - 0.01f));
	const float GridX0 = -0.5f * PX * NX;
	const float GridY0 = -0.5f * PY * NY;
	const FVector Scale(PX / CubeX, PY / CubeY, TZ / CubeZ);

	for (int32 I = 0; I < NX; ++I)
	{
		for (int32 J = 0; J < NY; ++J)
		{
			const float X0 = GridX0 + I * PX;
			const float Y0 = GridY0 + J * PY;

			// whole-tile knockout (1uu tolerance, matches BuildMeshTiles)
			bool bCut = false;
			for (const FBox2D& C : Cuts)
			{
				if (X0 + PX > C.Min.X + 1.f && X0 < C.Max.X - 1.f &&
					Y0 + PY > C.Min.Y + 1.f && Y0 < C.Max.Y - 1.f)
				{
					bCut = true;
					break;
				}
			}
			if (bCut) { continue; }

			// cube pivot is centered: place at cell center, top face at actor Z
			const FVector Pos(X0 + 0.5f * PX, Y0 + 0.5f * PY, -0.5f * TZ);
			SlabISM->AddInstance(FTransform(FRotator::ZeroRotator, Pos, Scale), /*bWorldSpace*/ false);
		}
	}
}
