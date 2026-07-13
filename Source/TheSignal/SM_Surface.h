// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * ASM_Surface
 *
 * Purpose: Procedural flat slab (floor / roof / ceiling) that cuts REAL holes —
 *          visibility AND collision — around actors the designer assigns in the
 *          Details panel (a specific SM_Stair, elevator shaft walls, ...).
 *          The cut follows those actors: the slab rebuilds automatically in the
 *          editor whenever an assigned actor moves or resizes. The material
 *          tiles in world units (AZP_TileSize = size of ONE pattern repeat), so
 *          a floor-tile material lays out as actual tiles instead of being
 *          stretched once across the whole surface.
 *
 *          Cut rule is deliberately dumb and predictable: each assigned actor's
 *          bounds footprint (plus AZP_CutMargin) is removed from the slab,
 *          regardless of Z — assigning an actor IS the statement of intent.
 *
 * Owner Subsystem: FacilitySystemsManager (level architecture helpers)
 *
 * Blueprint extension points: all AZP_ knobs are EditAnywhere/BlueprintReadWrite;
 *          Rebuild() is BlueprintCallable and CallInEditor.
 *
 * Dependencies: ProceduralMeshComponent (engine-default runtime plugin module).
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SM_Surface.generated.h"

class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

UCLASS(Blueprintable)
class THESIGNAL_API ASM_Surface : public AActor
{
	GENERATED_BODY()

public:
	ASM_Surface();

	/**
	 * LEGACY ROOT ONLY — renders and collides NOTHING anymore. The old
	 * procedural-mesh slab was retired 2026-07-12: PMC generates no runtime
	 * distance fields, so Lumen could not represent it (slab rendered PITCH
	 * BLACK in PIE) and CMC landing failed on its trimesh floor (post-dodge
	 * ice-slide, LaunchCharacter never re-landed). Kept as root so placed
	 * actors keep their serialized transforms. Do NOT build sections on it.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surface")
	TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

	/**
	 * MESH-TILE MODE lives on THIS component: select TileISM in the component
	 * tree and set its ordinary Static Mesh field (e.g. SM_Floor_4). The
	 * surface fills with a grid of instances of that mesh — every tile at the
	 * mesh's NATURAL size (never scaled or stretched). The grid always covers
	 * the FULL AZP_SurfaceSize rectangle, so swapping meshes never changes the
	 * shape: the partial row/column overhangs the edge by up to half a tile
	 * per side instead of scaling. Cut actors knock out whole tiles
	 * (visibility + collision). Clear the mesh to fall back to slab-tile mode.
	 * MATERIALS ARE YOURS TOO: this actor never writes a material anywhere —
	 * override them right here on the component (or on SlabISM for the slab)
	 * in the ordinary Materials slots, exactly like any placed mesh.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surface")
	TObjectPtr<UInstancedStaticMeshComponent> TileISM;

	/**
	 * SLAB-TILE MODE's geometry (active when TileISM has no mesh): a grid of
	 * engine-cube tiles, AZP_TileSize pitch, AZP_Thickness thick, top at the
	 * actor's Z — set your material on THIS component's Materials slot and
	 * every tile shows exactly ONE clean pattern repeat (uniform grid, no
	 * stretching). Real static meshes, so Lumen lights them and the character
	 * lands on them exactly like hand-placed floors.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surface")
	TObjectPtr<UInstancedStaticMeshComponent> SlabISM;

	/** Total slab size in uu (local X/Y), centered on the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "10.0"))
	FVector2D AZP_SurfaceSize = FVector2D(1000.f, 1000.f);

	/** Slab thickness in uu. The TOP face sits at the actor's Z. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface", meta = (ClampMin = "1.0"))
	float AZP_Thickness = 10.f;

	/** Actors whose bounds get cut out of this slab (visibility + collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Cuts")
	TArray<TObjectPtr<AActor>> AZP_CutActors;

	/** Extra margin (uu) around each cut actor's bounds. Negative shrinks the hole. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Cuts")
	float AZP_CutMargin = 0.f;

	/** Slab-tile mode only: world size (uu) of ONE tile cube = one material pattern repeat (e.g. 450x450 for the pack floor look). Mesh-tile mode ignores this — the pitch is always the mesh's own footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Material", meta = (ClampMin = "1.0"))
	FVector2D AZP_TileSize = FVector2D(450.f, 450.f);

	/** Generate walkable/blocking collision for the slab (holes have none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	bool bAZP_GenerateCollision = true;

	/** Seconds between checks for moved cut actors (editor live-follow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface|Cuts", meta = (ClampMin = "0.1"))
	float AZP_FollowInterval = 0.5f;

	/** Regenerate the slab now (also runs automatically on edit/move/follow). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Surface")
	void Rebuild();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

protected:
	virtual void BeginPlay() override;

private:
	float FollowAccum = 0.f;
	uint32 LastStateHash = 0;

	/** Hash of EVERYTHING the build depends on (cut bounds, TileISM mesh, all
	 *  AZP knobs) — the editor tick rebuilds on any change, so the actor needs
	 *  no scripts and no manual refresh, ever. */
	uint32 ComputeStateHash() const;
	TArray<FBox2D> GatherCutRects() const;
	void BuildSlabTiles(const TArray<FBox2D>& Cuts);
	void BuildMeshTiles(const TArray<FBox2D>& Cuts);
};
