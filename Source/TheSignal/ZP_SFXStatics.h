// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_SFXStatics
 *
 * Purpose: THE single playback path for world-positioned sound effects. Every 3D SFX in the game
 *          (enemy voices, impacts, doors, explosions) must route through PlaySFXAttached /
 *          PlaySFXAtLocation so it gets a proper long-carry attenuation. Playing a sound with no
 *          attenuation asset silently inherits UE5's default falloff (400 inner + 3600 falloff =
 *          DEAD SILENT past 40 m) — which is exactly the "I can't hear the shambler down the
 *          hallway" bug. Carry distances are C++-owned header constants (no .uasset to drift).
 *
 *          Also owns the project's listener-occlusion muffle (trace-based half-volume + low-pass).
 *          The ENGINE's SoundAttenuation occlusion is a confirmed dead end in this project — it
 *          reliably MUTES sounds outright (tested on both Visibility and WorldStatic channels), so
 *          it stays disabled here and the muffle is done manually.
 *
 * Owner Subsystem: Audio
 *
 * Blueprint extension points: both Play functions are BlueprintCallable so BP-side SFX (doors,
 * containers, pickups) can use the same carry profiles.
 *
 * Dependencies: none (pure statics; transient USoundAttenuation objects built on demand).
 */

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/HitResult.h"
#include "ZP_SFXStatics.generated.h"

class USoundBase;
class USoundAttenuation;
class UAudioComponent;
class USceneComponent;

/** How far a world SFX carries before falling silent. Natural (logarithmic) falloff — clearly
 *  present near the source, still audible at range, fading out toward the max distance. */
UENUM(BlueprintType)
enum class EZP_SFXCarry : uint8
{
	Close UMETA(ToolTip = "Small foley - impacts, handling, container rummage. ~30 m"),
	Room  UMETA(ToolTip = "Ordinary world SFX - doors, machines, footfalls of others. ~60 m"),
	Far   UMETA(ToolTip = "Enemy voices, screams, gunshots, explosions. ~80 m")
};

/** Broad surface family of a struck world surface — picks impact SFX variants (melee pipe today;
 *  reusable for bullet impacts / footsteps later). Small deliberately: a couple of families that
 *  cover the facility, Concrete as the catch-all (dev 2026-08-05). */
UENUM(BlueprintType)
enum class EZP_ImpactSurface : uint8
{
	Concrete UMETA(ToolTip = "Concrete/brick/plaster/stone - the facility default"),
	Metal    UMETA(ToolTip = "Vents, lockers, machines, doors, railings, pipes"),
	Wood     UMETA(ToolTip = "Crates, pallets, furniture, boards"),
	Glass    UMETA(ToolTip = "Glass, windows, tile, ceramic, screens")
};

UCLASS()
class THESIGNAL_API UZP_SFXStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ── Carry profiles (UU). TRUE inverse-square rolloff (-6 dB per distance doubling), anchored
	//    at InnerRadius — the real physical law, so moving toward/away from a source produces the
	//    continuous differential real ears expect. Full volume only inside InnerRadius (~1.5 m);
	//    half amplitude at 2x inner, quarter at 4x, and so on. The profiles differ mainly in how
	//    far they stay mixed before going sub-audible. 100 UU = 1 m.
	static constexpr float AZP_CloseInnerRadius = 100.f;
	static constexpr float AZP_CloseFalloff     = 2900.f;   // sub-audible past ~30 m
	static constexpr float AZP_RoomInnerRadius  = 150.f;
	static constexpr float AZP_RoomFalloff      = 5850.f;   // sub-audible past ~60 m
	static constexpr float AZP_FarInnerRadius   = 150.f;
	static constexpr float AZP_FarFalloff       = 7850.f;   // sub-audible past ~80 m

	// ── Distance feel: air absorption + reverb ratio (why "far" actually SOUNDS far) ──
	// Distance character indoors = QUIETER + WETTER (more hallway tail), with a gentle top-end
	// shelf. Heavy low-pass belongs ONLY to through-wall transmission — never plain distance.
	static constexpr float AZP_LPFStartFraction   = 0.35f;    // top-end shelf starts past this fraction of max range
	static constexpr float AZP_LPFFrequencyAtFar  = 7000.f;   // Hz at max range — a shelf, not a blanket
	static constexpr float AZP_ReverbWetNear      = 0.08f;    // subtle tail on close sounds
	static constexpr float AZP_ReverbWetFar       = 0.7f;     // distant sounds are echo-dominant

	// ── Facility reverb bed (concrete corridor). Auto-activated once per world on the first
	//    world SFX, so every reverb send above actually renders. AudioVolumes with higher
	//    priority can override per-room later; this is the level-wide default space. ──
	static constexpr float AZP_ReverbMasterVolume     = 0.35f;
	static constexpr float AZP_ReverbDecayTime        = 2.3f;   // s — long concrete tail
	static constexpr float AZP_ReverbDecayHFRatio     = 0.55f;  // concrete keeps the low end ringing
	static constexpr float AZP_ReverbGain             = 0.32f;
	static constexpr float AZP_ReverbGainHF           = 0.55f;
	static constexpr float AZP_ReverbReflectionsGain  = 0.14f;
	static constexpr float AZP_ReverbReflectionsDelay = 0.012f; // s — hallway-width early slap
	static constexpr float AZP_ReverbLateGain         = 1.1f;
	static constexpr float AZP_ReverbLateDelay        = 0.02f;
	static constexpr float AZP_ReverbDiffusion        = 0.85f;
	static constexpr float AZP_ReverbDensity          = 1.0f;

	// ── Propagation (manual — engine occlusion is a project dead end, see header) ──
	// CONTINUOUS model, re-evaluated for the whole life of every sound by
	// UZP_SFXPropagationSubsystem (never a spawn-time snapshot — sounds respond live as the player
	// moves). Three regimes blending into each other:
	//   DIRECT      — clear sightline: dry, untouched.
	//   DIFFRACTED  — sightline blocked but an OPEN ROUTE exists (navmesh path around the corner /
	//                 down the connecting hall): sound bends — level drops CONTINUOUSLY with the
	//                 detour length (ratio 1.0 = indistinguishable from Direct), top end mostly
	//                 SURVIVES. Never a pillow-muffle.
	//   TRANSMITTED — no open route: genuinely through a wall. Heavy muffle IS the correct physics.
	/** Detour ratio (nav path length / straight line) above which "around the corner" stops being
	 *  believable and the sound counts as through-wall. */
	static constexpr float AZP_MaxDiffractionDetour   = 2.4f;
	// RETUNED 2026-08-07 (dev: shambler lurk "clearly" audible from the next room; measured tier
	// was Diffracted at ratio 1.43 -> 0.85 vol / 11.4 kHz — effectively transparent for a growl,
	// whose energy sits far below any of these filters). A BLOCKED SIGHTLINE must never sound
	// like the same room: even a near-straight open route through an aperture starts at a clear
	// "other room" level now. Direct tier is untouched.
	static constexpr float AZP_DiffractedVolumeMin    = 0.65f;  // detour ratio ~1: audible, but unmistakably around a corner
	static constexpr float AZP_DiffractedVolumeMax    = 0.3f;   // long detour, several corners
	static constexpr float AZP_DiffractedLPFMinHz     = 7000.f;
	static constexpr float AZP_DiffractedLPFMaxHz     = 1800.f;
	/** ONE-BEND aperture (2026-08-05: "muffled until I stepped through the door"): when some point
	 *  on the open-air route can see BOTH the listener and the source, the sound bends exactly once
	 *  (an open doorway / single corner) and arrives nearly clean — the detour RATIO is meaningless
	 *  there (a doorway route can read 2x+ while acoustically wide open). Near-full level, barely
	 *  audible filtering. */
	// RETUNED 2026-08-07 with the diffraction bands: an open door LEAKS — noticeably louder and
	// brighter than the through-wall thump (0.3 / 500 Hz) — but the sound still reads as "coming
	// from the next room", never as sharing your room. (Was 0.92 / 12 kHz = near-transparent.)
	static constexpr float AZP_OneBendVolume          = 0.6f;
	static constexpr float AZP_OneBendLPFHz           = 5000.f;
	/** Height (UU) above a nav path point for the one-bend pivot probe — mid-door-opening. */
	static constexpr float AZP_OneBendPivotHeight     = 80.f;
	static constexpr float AZP_TransmittedVolumeScale = 0.3f;
	static constexpr float AZP_TransmittedLowPassHz   = 500.f;
	/** RETIRED 2026-08-07 (kept for the record): the "blocker hugging the source is its own
	 *  perch" exemption. Built for the DEPRECATED wall-clinging crawler; caused full-volume
	 *  leaks three ways as it shrank (150: enemies near thin walls; 40: an enemy pressed against
	 *  the far side of a closed door). Occlusion now counts ANY geometry between listener and
	 *  source — the enemy's own body is excluded via IgnoreActor. Do not resurrect; if a future
	 *  climber needs to be heard through its perch, solve it with a per-sound bPropagate=false
	 *  or a surface-identity check, never a distance skin. */
	static constexpr float AZP_OcclusionSelfSkin      = 0.f;

	/** Play a world SFX following a component (enemy voice, machine hum burst, etc.).
	 *  bPropagate: run the 3-tier Direct/Diffracted/Transmitted model (default ON for every world
	 *  SFX — this is what makes the map "carry" uniformly with zero per-sound work). */
	UFUNCTION(BlueprintCallable, Category = "SFX")
	static UAudioComponent* PlaySFXAttached(USoundBase* Sound, USceneComponent* AttachTo,
		EZP_SFXCarry Carry = EZP_SFXCarry::Room, float Volume = 1.f, float Pitch = 1.f,
		bool bPropagate = true, float ForceLowPassHz = 0.f);

	/** Play a world SFX at a fixed spot (explosion, thrown-object impact, distant event). */
	UFUNCTION(BlueprintCallable, Category = "SFX", meta = (WorldContext = "WorldContextObject"))
	static UAudioComponent* PlaySFXAtLocation(const UObject* WorldContextObject, USoundBase* Sound,
		FVector Location, EZP_SFXCarry Carry = EZP_SFXCarry::Room, float Volume = 1.f, float Pitch = 1.f,
		bool bPropagate = true);

	/** True if solid WorldStatic geometry sits between the listener (player viewpoint) and SourceLoc. */
	static bool IsOccludedFromListener(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor);

	/** Classify the surface family a trace/sweep struck, for impact-SFX selection (melee pipe today).
	 *  Priority: the hit's PhysicalMaterial name (authored ground truth when present — requires
	 *  bReturnPhysicalMaterial on the query), then a keyword scan over the hit component's material
	 *  slot names + mesh asset name + component/actor names — works on purchased-pack content with
	 *  zero per-asset setup. Unmatched = Concrete (the facility default). */
	UFUNCTION(BlueprintCallable, Category = "SFX")
	static EZP_ImpactSurface ClassifySurface(const FHitResult& Hit);

	/** 3-tier propagation: writes the volume multiplier and low-pass (0 = none) for a source at
	 *  SourceLoc as heard by the player. Direct = 1.0/none; Diffracted = detour-scaled level drop
	 *  with mild HF loss (navmesh path exists around the blocker); Transmitted = through-wall muffle.
	 *  bOutTransmitConfident (optional): true ONLY when Transmitted was proven by a valid full nav
	 *  path with a huge detour — false when the route was merely unknowable (pawn off-navmesh etc.).
	 *  Gameplay callers (sight-aggro veto) must require confidence; audio can muffle either way. */
	static void ComputePropagation(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
		float& OutVolumeMul, float& OutLowPassHz, bool* bOutTransmitConfident = nullptr);

	/** The uncached body of ComputePropagation (occlusion trace + navmesh diffraction pathfind).
	 *  ComputePropagation memoizes it per (source-cell, listener-cell) for 0.5s — every occluded
	 *  enemy footstep used to re-run a SYNC pathfind from effectively the same spots (perf 2026-08-04). */
	static void ComputePropagationUncached(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
		float& OutVolumeMul, float& OutLowPassHz, bool* bOutTransmitConfident = nullptr);

	/** The shared, transient attenuation object for a carry profile (built once, reused, rooted). */
	static USoundAttenuation* GetCarryAttenuation(EZP_SFXCarry Carry);

	/** Activate the facility reverb bed on this world (idempotent; called lazily by both Play
	 *  functions). BlueprintCallable so a level BP can also force it on before any SFX plays. */
	UFUNCTION(BlueprintCallable, Category = "SFX", meta = (WorldContext = "WorldContextObject"))
	static void EnsureWorldReverb(const UObject* WorldContextObject);
};
