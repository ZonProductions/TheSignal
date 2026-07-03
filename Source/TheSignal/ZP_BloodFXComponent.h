// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_BloodFXComponent
 *
 * Purpose: Per-enemy blood identity. Weapons (melee sweep, hitscan) call the static
 *          PlayHitBloodFor() on whatever they hit; if the actor carries this component its values
 *          are used, otherwise the class defaults apply — so EVERY enemy bleeds out of the box and
 *          any enemy BP can override everything by adding + editing this component.
 *
 *          A hit is a COMPOSITE, not one particle puff (dev: "real visceral effect"):
 *            1. Directional burst (pack Niagara) along the hit direction.
 *            2. Heavy hits (melee bludgeon) add a big-splash system on top.
 *            3. GUARANTEED persistent splatter: C++ traces from the wound — along the spray to the
 *               wall behind, and down to the floor — and stamps tinted DECALS that stay. Never
 *               relies on particle collisions (which is why the first pass "just disappeared").
 *            4. Heavy hits attach a short bleed dribble to the enemy body.
 *
 * Owner Subsystem: Combat / VFX
 *
 * Blueprint extension points: add to any enemy BP and edit the Blood category per enemy
 * (colors, systems, scale, splatter counts/sizes).
 * Dependencies: Niagara module; /Game/Blood_VFX_Pack (Rimaye.Std, UE 5.8).
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_BloodFXComponent.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_BloodFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_BloodFXComponent();

	// ── Systems (lazily defaulted to the Blood_VFX_Pack assets, dev-picked off the overview map) ──
	/** Spurt intensity 1-3 — the pack ships every hit system in three levels (the three mannequins
	 *  per stall on the overview map). THE per-enemy dial: set it on BloodFX in BP_Shambler /
	 *  BP_Scytheer. Picks MeleeBloodSystems/RangedBloodSystems[Intensity-1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", meta = (ClampMin = "1", ClampMax = "3"))
	int32 BloodIntensity = 2;

	/** MELEE hit systems by intensity ([0]=1 .. [2]=3). Default: the overview map's
	 *  "Splash with Burst + Hit" trio (P_SplashWithBurst_Hit_01/02/03) — correct residual blood. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	TArray<TObjectPtr<UNiagaraSystem>> MeleeBloodSystems;

	/** RANGED hit systems by intensity. Default: the overview map's "Splash + Hit + Metal" trio
	 *  (P_Splash_HitWithMetal_01/02/03) — bullet impact/ricochet read. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	TArray<TObjectPtr<UNiagaraSystem>> RangedBloodSystems;

	/** OPTIONAL dribble attached to the body after a heavy hit. Default NONE — the pack's bleeding
	 *  systems spurt like a halloween gag (dev-rejected). Assign only if a genuinely subtle drip
	 *  system exists. The quiet aftermath is the delayed floor pool below instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	TObjectPtr<UNiagaraSystem> BleedSystem;

	/** Seconds the attached bleed runs before deactivating (only if BleedSystem is set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	float BleedDuration = 1.6f;

	// ── Quiet aftermath: a small pool forming under the enemy after a heavy hit ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	bool bDelayedFloorPool = true;

	/** Seconds after the heavy hit before the pool appears (blood had time to run down). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float PoolDelay = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float PoolSizeMin = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float PoolSizeMax = 44.f;

	/** Stop the owner's skeletal meshes RECEIVING decals. ON by default — DEAD END, proven 3 ways
	 *  in PIE (2026-07-02): world-space decals float in air when the body moves; bone-attached
	 *  decals smear as surfaces move through the projection box mid-swing; on death the attached
	 *  box rides the spine to the ground and paints the FLOOR. Decals cannot stick to deforming
	 *  skin. Real on-body blood = material wound masks (future feature). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	bool bDisableBodyDecals = true;

	// ── Body WOUNDS that cling (pre-skinned sphere-mask in M_ZP_CreatureSkin — the free
	//    L4D2/Looman technique; wounds live in reference-pose space inside the skin shader, so
	//    they follow every animation and the death fall; zero UV requirements) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Wounds")
	bool bBodyWounds = true;

	/** Wound tint blended into the skin's BaseColor (roughness also drops to a wet 0.3 inside). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Wounds")
	FLinearColor WoundColor = FLinearColor(0.06f, 0.008f, 0.1f, 1.f);

	/** Wound radius in WORLD units (converted to mesh-local for the shader). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Wounds")
	float WoundRadiusMin = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Wounds")
	float WoundRadiusMax = 24.f;

	/** MUST match the WoundLoc_N/WoundRadius_N slot count in M_ZP_CreatureSkin (4). Oldest wound
	 *  is overwritten when full — enemies die in ~4 hits, so the cap never visibly recycles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Wounds")
	int32 MaxBodyWounds = 4;

	// ── Body residual decals — DEAD END, default OFF (see bDisableBodyDecals). Kept only as an
	//    experiment knob; enabling also requires bDisableBodyDecals=false. ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter", AdvancedDisplay)
	bool bBodyResidual = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter", AdvancedDisplay)
	int32 NumBodyResiduals = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter", AdvancedDisplay)
	float BodyResidualSizeMin = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter", AdvancedDisplay)
	float BodyResidualSizeMax = 14.f;

	// ── Colors ──
	/** Particle blood tint. Dark PURPLE that still READS in dark rooms — the first-pass near-black
	 *  (2% luminance) was invisible, which killed the whole effect. Per-enemy override in BP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	FLinearColor BloodColor = FLinearColor(0.14f, 0.015f, 0.22f, 1.f);

	/** Mist/smoke layer tint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	FLinearColor SmokeColor = FLinearColor(0.06f, 0.02f, 0.1f, 1.f);

	/** Splatter DECAL tint — inkier/darker than the airborne blood (drying ink look on surfaces). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	FLinearColor DecalColor = FLinearColor(0.05f, 0.006f, 0.085f, 1.f);

	// ── Scale / feel ──
	/** Overall burst scale (component scale AND the "Scale" user param). A 1 m lead pipe is not a
	 *  water balloon — default is deliberately big. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	float BloodScale = 1.6f;

	/** Particle lifetime multiplier ("LifeTimeMult" user param) — longer-lingering droplets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood")
	float LifeTimeMult = 1.5f;

	// ── Guaranteed splatter decals (C++ traces — deterministic, they always land) ──
	/** Decal material. Default: pack MI_BloodDecal (master exposes a "Color" vector param). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	TObjectPtr<UMaterialInterface> DecalMaterial;

	/** Splats traced along the spray direction onto the wall/geometry BEHIND the enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	int32 NumWallSplats = 2;

	/** Splats traced DOWN onto the floor around/past the enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	int32 NumFloorSplats = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float DecalSizeMin = 26.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float DecalSizeMax = 58.f;

	/** Seconds before a splatter decal fades. 0 = NEVER — it stays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float DecalLifetime = 0.f;

	/** How far behind the wound the wall-splatter trace reaches (UU). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood|Splatter")
	float WallTraceDistance = 420.f;

	// ── Pack user-param names (only touch if a swapped system names them differently) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", AdvancedDisplay)
	FName ColorParam = FName(TEXT("Color"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", AdvancedDisplay)
	FName SmokeColorParam = FName(TEXT("SmokeColor"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", AdvancedDisplay)
	FName ScaleParam = FName(TEXT("Scale"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", AdvancedDisplay)
	FName LifeTimeParam = FName(TEXT("LifeTimeMult"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood", AdvancedDisplay)
	FName DecalColorParam = FName(TEXT("Color"));

	/** Full composite at Location spraying along Direction. bMeleeHit selects the melee system trio
	 *  (+ delayed floor pool); false = the ranged/bullet trio. SurfaceNormal (the wound's impact
	 *  normal) orients the body-residual stains flat against the skin — pass Hit.ImpactNormal;
	 *  ZeroVector falls back to the spray direction. ExtraIgnoreActor: a second body the splatter
	 *  traces must pass through — the GRABBER during the grab bite (its Pawn capsule blocks the
	 *  WorldStatic trace 36uu out; the movement-only collision ignore does NOT cover line traces,
	 *  so without this every wall splat stamps invisibly on the capsule). */
	UFUNCTION(BlueprintCallable, Category = "Blood")
	void PlayHitBlood(const FVector& Location, const FVector& Direction, bool bMeleeHit = true,
		FVector SurfaceNormal = FVector::ZeroVector, AActor* ExtraIgnoreActor = nullptr);

	/** Weapon-side entry point. Uses HitActor's component if present, else class defaults. */
	static void PlayHitBloodFor(AActor* HitActor, const FVector& Location, const FVector& Direction,
		bool bMeleeHit = true, FVector SurfaceNormal = FVector::ZeroVector, AActor* ExtraIgnoreActor = nullptr);

protected:
	/** Preloads assets, disables body decal reception, and prewarms shaders — the whole effect
	 *  must be paid for AT LEVEL LOAD, not on the first swing (dev report: first hits showed the
	 *  default-material checkerboard + a lag spike while shaders compiled). */
	virtual void BeginPlay() override;

private:
	struct FBloodSpawnParams; // all resolved values for one composite spawn
	static void SpawnComposite(UWorld* World, AActor* HitActor, const FVector& Location,
		const FVector& Direction, bool bMeleeHit, const FVector& SurfaceNormal, const FBloodSpawnParams& P,
		AActor* ExtraIgnoreActor = nullptr);
	static void SpawnTintedSystem(UWorld* World, UNiagaraSystem* System, const FVector& Location,
		const FRotator& Rotation, const FBloodSpawnParams& P);
	static void SpawnSplatterDecals(UWorld* World, AActor* HitActor, const FVector& Origin,
		const FVector& Direction, const FBloodSpawnParams& P, AActor* ExtraIgnoreActor = nullptr);
	static void StampDecalAt(UWorld* World, const FHitResult& Hit, const FBloodSpawnParams& P,
		float SizeMin, float SizeMax);
	static void SpawnBodyResidual(AActor* HitActor, const FVector& Location, const FVector& ProjectDir,
		const FBloodSpawnParams& P);

	/** Once per world: spawn each system + a decal far below the map at tiny scale so Niagara
	 *  render resources, materials, and PSOs compile during load instead of mid-combat. */
	static void PrewarmWorldOnce(UWorld* World);

	/** Stamp a clinging wound: world hit -> current-bone local -> REFERENCE-POSE space, written to
	 *  the skin MIDs' WoundLoc_N/WoundT_N/WoundB_N/WoundRadius_N params. M_ZP_CreatureSkin projects
	 *  the pack's HUMAN-PAINTED decal texture (WoundTex, default T_BloodDecal — same art as the
	 *  wall splats) on the T/B plane, enveloped by a sphere mask vs Pre-Skinned Local Position.
	 *  SurfaceNormal orients the projection (random roll for variety). Round-robin slots. */
	void ApplyBodyWound(AActor* HitActor, const FVector& WorldLocation, const FVector& SurfaceNormal);

	void EnsureAssets();
	static UNiagaraSystem* LoadSystem(const TCHAR* Path);

	int32 NextWoundSlot = 0;
	bool bWoundMIDsCreated = false;
};
