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
	static constexpr float CloseInnerRadius = 100.f;
	static constexpr float CloseFalloff     = 2900.f;   // sub-audible past ~30 m
	static constexpr float RoomInnerRadius  = 150.f;
	static constexpr float RoomFalloff      = 5850.f;   // sub-audible past ~60 m
	static constexpr float FarInnerRadius   = 150.f;
	static constexpr float FarFalloff       = 7850.f;   // sub-audible past ~80 m

	// ── Distance feel: air absorption + reverb ratio (why "far" actually SOUNDS far) ──
	// Distance character indoors = QUIETER + WETTER (more hallway tail), with a gentle top-end
	// shelf. Heavy low-pass belongs ONLY to through-wall transmission — never plain distance.
	static constexpr float LPFStartFraction   = 0.35f;    // top-end shelf starts past this fraction of max range
	static constexpr float LPFFrequencyAtFar  = 7000.f;   // Hz at max range — a shelf, not a blanket
	static constexpr float ReverbWetNear      = 0.08f;    // subtle tail on close sounds
	static constexpr float ReverbWetFar       = 0.7f;     // distant sounds are echo-dominant

	// ── Facility reverb bed (concrete corridor). Auto-activated once per world on the first
	//    world SFX, so every reverb send above actually renders. AudioVolumes with higher
	//    priority can override per-room later; this is the level-wide default space. ──
	static constexpr float ReverbMasterVolume     = 0.35f;
	static constexpr float ReverbDecayTime        = 2.3f;   // s — long concrete tail
	static constexpr float ReverbDecayHFRatio     = 0.55f;  // concrete keeps the low end ringing
	static constexpr float ReverbGain             = 0.32f;
	static constexpr float ReverbGainHF           = 0.55f;
	static constexpr float ReverbReflectionsGain  = 0.14f;
	static constexpr float ReverbReflectionsDelay = 0.012f; // s — hallway-width early slap
	static constexpr float ReverbLateGain         = 1.1f;
	static constexpr float ReverbLateDelay        = 0.02f;
	static constexpr float ReverbDiffusion        = 0.85f;
	static constexpr float ReverbDensity          = 1.0f;

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
	static constexpr float MaxDiffractionDetour   = 2.4f;
	static constexpr float DiffractedVolumeMin    = 1.0f;   // detour ratio ~1: same as Direct (seamless blend)
	static constexpr float DiffractedVolumeMax    = 0.5f;   // long detour, several corners
	static constexpr float DiffractedLPFMinHz     = 15000.f; // inaudible filtering at ratio ~1
	static constexpr float DiffractedLPFMaxHz     = 3200.f;
	static constexpr float TransmittedVolumeScale = 0.3f;
	static constexpr float TransmittedLowPassHz   = 500.f;
	/** A blocker within this many UU of the source is the source's OWN perch/contact geometry
	 *  (e.g. a crawler clinging to the wall it's heard through), not a wall between rooms. */
	static constexpr float OcclusionSelfSkin      = 150.f;

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

	/** 3-tier propagation: writes the volume multiplier and low-pass (0 = none) for a source at
	 *  SourceLoc as heard by the player. Direct = 1.0/none; Diffracted = detour-scaled level drop
	 *  with mild HF loss (navmesh path exists around the blocker); Transmitted = through-wall muffle.
	 *  bOutTransmitConfident (optional): true ONLY when Transmitted was proven by a valid full nav
	 *  path with a huge detour — false when the route was merely unknowable (pawn off-navmesh etc.).
	 *  Gameplay callers (sight-aggro veto) must require confidence; audio can muffle either way. */
	static void ComputePropagation(UWorld* World, const FVector& SourceLoc, const AActor* IgnoreActor,
		float& OutVolumeMul, float& OutLowPassHz, bool* bOutTransmitConfident = nullptr);

	/** The shared, transient attenuation object for a carry profile (built once, reused, rooted). */
	static USoundAttenuation* GetCarryAttenuation(EZP_SFXCarry Carry);

	/** Activate the facility reverb bed on this world (idempotent; called lazily by both Play
	 *  functions). BlueprintCallable so a level BP can also force it on before any SFX plays. */
	UFUNCTION(BlueprintCallable, Category = "SFX", meta = (WorldContext = "WorldContextObject"))
	static void EnsureWorldReverb(const UObject* WorldContextObject);
};
