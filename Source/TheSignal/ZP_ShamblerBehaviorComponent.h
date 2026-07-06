// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ShamblerBehaviorComponent
 *
 * Purpose: Ground-zombie ("Shambler") AI. Wanders a small radius until it gets a clear line of sight
 *          to the player within AZP_DetectionRange, screams (alert), then chases. Attacks (alternating
 *          left/right swipe) when in range. Gives up and returns to wandering if it loses line of
 *          sight / the player leaves its space for AZP_LoseSightTime.
 *
 *          Drives the owning Character's AIController (MoveTo) for navigation, and plays animations
 *          directly via the mesh in SingleNode mode (walk / idle / attack / scream) — no AnimGraph
 *          wiring required. Reuses UZP_EnemyAudioComponent for spatial lurk/alert/attack voice and
 *          UZP_HealthComponent for death.
 *
 * Owner Subsystem: EnemyAI
 *
 * Add to any Character (e.g. BP_Shambler). All movement/anim/audio is self-contained.
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"                       // FAIRequestID
#include "Navigation/PathFollowingComponent.h" // EPathFollowingResult
#include "ZP_Staggerable.h"
#include "ZP_Revivable.h"
#include "ZP_Grabbable.h"
#include "ZP_ShamblerBehaviorComponent.generated.h"

class UAnimSequence;
class UAnimMontage;
class USoundBase;
class UAudioComponent;
class UZP_EnemyAudioComponent;
class UZP_HealthComponent;
class UDamageType;
class UPrimitiveComponent;
class ACharacter;
class AAIController;
class AController;

UENUM(BlueprintType)
enum class EShamblerState : uint8
{
	Wander,   // roaming, unaware
	Scream,   // just spotted the player — playing the alert
	Chase,    // pursuing
	Attack,   // mid-swipe
	Grab      // latched onto the player — paired grab/struggle (victim-driven, see IZP_Grabber)
};

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_ShamblerBehaviorComponent : public UActorComponent, public IZP_Staggerable, public IZP_Revivable, public IZP_Grabber
{
	GENERATED_BODY()

public:
	UZP_ShamblerBehaviorComponent();

	// --- Detection ---
	/** Max distance (UU) at which it can notice the player. Real walls/doors are meant to gate aggro via
	 *  line of sight, NOT this range. Left at the original 1800; let LOS do the containing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float AZP_DetectionRange = 1800.f;

	/** Seconds without a clear sightline to the player before it gives up and wanders off.
	 *  This is the "wait at the shut door" beat — it pursues to the last spot, then resets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float AZP_LoseSightTime = 5.f;

	/** Hard leash (UU): beyond this it gives up immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float AZP_GiveUpRange = 3000.f;

	/** dot(forward, dirToPlayer) it must exceed to notice you — it has to be roughly facing you.
	 *  0.25 ~ a 75-degree half-cone in front. Lower = wider awareness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float AZP_FacingThreshold = 0.25f;

	/** Point-blank distance (UU) within which the Shambler senses the player regardless of facing
	 *  (also bypasses the through-wall audio veto) — invented name for the hardcoded 350.f in the
	 *  wander sight check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float AZP_CloseSenseRange = 350.f;

	// --- Damage / death (player shooting the Shambler) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float AZP_MaxHealth = 100.f;

	/** Damage per body shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float AZP_BodyShotDamage = 20.f;

	/** Damage per headshot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float AZP_HeadShotDamage = 50.f;

	/** A hit this many UU above the body centre counts as a headshot (capsule hits carry no bone, so
	 *  headshots are judged by height). Capsule half-height is ~88; ~55 ≈ upper chest/head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float AZP_HeadshotMinZ = 55.f;

	// --- Attack ---
	/** Distance (UU) at which it COMMITS to a swing. Larger than the contact radius on purpose — it
	 *  starts swinging while you still have space, so you see swings often and get a window to block
	 *  or back-step. (Was 160: it had to park on your toes before ever swinging, so swings were rare.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackRange = 230.f;

	/** Distance (UU) the player must still be within AT THE MOMENT the hit lands. Decoupled from
	 *  AZP_AttackRange: back-stepping during the wind-up makes the swing whiff — that's the defensive
	 *  counterplay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackHitRange = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackCooldown = 0.4f;

	/** HALVED 25 -> 12.5 (dev 2026-07-03: "feels very overpowered, like PIE is set to very
	 *  hard" — real difficulty tiers come later). Four eaten swings now cost half the player's
	 *  health; block (x0.25 + counter-stagger) and the back-step whiff stay the smart plays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackDamage = 12.5f;

	/** CLIP time (s) of the contact frame in the swipe animation. Real-world hit timing is remapped
	 *  through AZP_WindupPlayRate/AZP_StrikePlayRate at swing start. Must be < the clip length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackHitTime = 1.3f;

	/** FALLBACK swing length (s) if a swing clip is missing. When the clip exists, the real duration
	 *  is computed per-swing from the clip length + the two play rates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_AttackDuration = 1.7f;

	/** CLIP time (s) where the rear-back (wind-up) ends and the strike launches. The clip plays at
	 *  AZP_WindupPlayRate up to here, then snaps to AZP_StrikePlayRate — slow telegraph, fast release. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_WindupEndTime = 1.0f;

	/** Play rate of the wind-up portion. Below 1 = slower, more readable rear-back (the "block now"
	 *  telegraph). 0.8 gives ~1.25 s of visible wind-up before the strike releases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_WindupPlayRate = 0.8f;

	/** Play rate of the strike + recovery portion. Above 1 = the swing snaps once released and the
	 *  next swing chains sooner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_StrikePlayRate = 1.6f;

	// --- Grab (paired grab/struggle — Docs/Plan_GrabStruggle.md) ---
	/** Distance (UU) at which it latches. THE GRAB IS THE OPENER (dev direction 2026-07-02): the
	 *  first thing it does in reach is grab — melee is the fallback while the grab is on cooldown
	 *  or was deflected/evaded. Matches AZP_AttackRange so the grab wins at the swing-commit distance
	 *  (and the Attack chain re-checks it between swings — see Evaluate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabRange = 230.f;

	/** Seconds after a LANDED grab before THIS zombie may grab again — the anti-chain-grab rule
	 *  (Condemned's instant re-grab is the documented anti-pattern). This is the window in which
	 *  it melees instead. Failed attempts use AZP_GrabFailCooldown instead (dev 2026-07-03). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabCooldown = 30.f;

	/** Seconds after a FAILED grab attempt (deflected by the block, or evaded via dodge/immunity/
	 *  menus) before the next try. Kept short so a read block doesn't buy a 30s grab-free fight.
	 *  Clamped to AZP_GrabCooldown at use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabFailCooldown = 6.f;

	/** Vertical counterpart to AZP_GrabPairDistance: nudges the zombie MESH up/down (UU) for the
	 *  duration of the grab so the paired bodies line up in height (Marcus runs at 0.869 scale).
	 *  Negative = down. Same mechanism as AZP_ScreamMeshZOffset; restores itself on state exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabPairZOffset = 0.f;

	/** Chest-to-chest spacing (UU) the zombie snaps to at latch — the authored pair distance of the
	 *  NAAT clips. Tune in PIE until hands land on the body (MarcusBody runs at 0.869 scale, so the
	 *  mannequin-authored spacing needs a nudge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabPairDistance = 70.f;

	/** Seconds the body LUNGES into AZP_GrabPairDistance at the latch. Grabbing from max reach
	 *  (AZP_GrabRange 230) used to TELEPORT it ~160uu in one frame — the visible pop the dev
	 *  reported ("latches on from the farthest reach... jerks", 2026-07-03). Ease-out slide,
	 *  driven in TickComponent while the pair collision is ignored. 0 = the old teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabSnapInDuration = 0.2f;

	/** Escape stumble-back — a SMALL, contact-timed step, not a launch and not a cross-room slide
	 *  (dev sequence 2026-07-02: camera is already back in 1P; the push reads, THEN the zombie
	 *  stumbles back ~half a meter and is ready to attack). The zombie HOLDS the grapple spacing
	 *  for AZP_EscapePushbackDelay seconds — dial this until the movement starts exactly at Marcus's
	 *  push/kick CONTACT in the 1P view — then steps back AZP_EscapePushbackDistance over
	 *  AZP_EscapePushbackDuration with an ease-out, synced to the stumble clip's back-steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_EscapePushbackDelay = 0.9f;

	/** DISPLACEMENT (UU) of the stumble-back — how far the body actually moves. Dev spec: half a
	 *  meter or less (50 = 0.5m). From the 70uu grapple spacing this lands ~115uu from the player
	 *  — past the capsule-clear point (~100) so the deferred collision restore still completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_EscapePushbackDistance = 45.f;

	/** Seconds the eased step-back takes. Stumble-paced, not shove-paced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_EscapePushbackDuration = 0.45f;

	/** MINIMUM stun after being kicked/pushed off (the escape-reward punish window). The real pause
	 *  is max(this, the Kicked/Pushed clip length ~2.3s) so it never resumes mid-reaction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_EscapeStunDuration = 1.5f;

	/** Stagger applied when a BLOCKING player deflects the grab attempt (mirrors the block reward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_DeflectStaggerDuration = 1.2f;

	/** Additive BONE-LOCAL rotations on the Shambler's upper arms during the grapple — dial out
	 *  arm clipping against Marcus, live in PIE. Applied by ABP_Shambler's C++ parent
	 *  (UZP_ShamblerGrabPoseAnimInstance); inert outside the Grab state. Zero = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	FRotator AZP_GrabArmLRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	FRotator AZP_GrabArmRRotation = FRotator::ZeroRotator;

	/** Additive BONE-LOCAL rotation on the Shambler's HEAD during the grapple — angle the mouth
	 *  INTO Marcus's neck (the pack pose reads "more like a hug than a bite", dev 2026-07-03).
	 *  Same mechanism/knob behavior as the arm rotations; dial live in PIE. Zero = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	FRotator AZP_GrabHeadRotation = FRotator::ZeroRotator;

	/** Pair spacing (UU) the zombie re-snaps to the instant a kick/push ESCAPE begins, so the
	 *  push contact reads (Marcus's arms reach the chest) before the knockback launches.
	 *  0 = off (keep the wrestle spacing). Dial live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	float AZP_GrabEscapeSnapDistance = 0.f;

	/** Looping snarl for the whole grapple — starts at the latch, hard-cut the instant the grab
	 *  ends on ANY path. Lazily defaults to /Game/Audio/Shambler/SFX_SHAMBLER_GRAB (looping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<USoundBase> AZP_GrabLoopSound;

	/** One-shot alert sting fired the instant the grab LATCHES (dev 2026-07-03), layered under
	 *  the snarl loop. Lazily defaults to /Game/Audio/Shambler/SFX_GRAB_ALERT in BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<USoundBase> AZP_GrabAlertSound;

	/** Seconds it holds the scream (stationary, plays the alert) before breaking into the chase.
	 *  Applies to SIGHT aggro — the cinematic beat when it spots you across a room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_ScreamHoldTime = 2.0f;

	/** Scream hold when aggro came from TAKING DAMAGE (or damage lands mid-scream). Short — a
	 *  point-blank attacker must not get a free 2 s wail to wale on; it snaps into the fight.
	 *  This is what makes melee spam trade instead of farming a stationary target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AZP_HurtScreamHoldTime = 0.5f;

	// --- Speeds (set on the owner's CharacterMovement) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_WanderSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_ChaseSpeed = 250.f;

	/** How far inside AZP_AttackRange the chase MoveToActor stops (acceptance = max(AZP_AttackRange
	 *  - 70, 40)) so it reliably crosses the swing threshold instead of parking at the edge —
	 *  invented name for the hardcoded 70.f (and 40.f floor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_ChaseAcceptanceInset = 70.f;

	/** SPRINT-CHASE (dev 2026-07-03: "I don't even need to dodge to get out of the way"). When the
	 *  visible chase target is farther than AZP_RunTriggerDistance, the Shambler breaks into the NAAT
	 *  run cycle at this speed until it's back on top of the player (inside AZP_AttackRange) or loses
	 *  sight. CMC MaxWalkSpeed while sprinting — dial live in PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_RunSpeed = 420.f;

	/** Distance (UU) at which the chase breaks into the run. Inside AZP_AttackRange it drops back to
	 *  the walk/attack flow — the gap between the two is the hysteresis band (no flapping). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_RunTriggerDistance = 450.f;

	/** Seconds of each RUN burst inside the sprint-chase. The sprint OPENS with a burst, then
	 *  alternates burst -> fast walk -> burst... for as long as the sprint band holds
	 *  (dev 2026-07-03: interspersed run/fast-walk, ~1:2). Min 0.25 (one eval tick). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_RunBurstDuration = 2.0f;

	/** Seconds of fast walk (AZP_ChaseSpeed on the walk BlendSpace) between run bursts.
	 *  0 = no walk phase, continuous running (the pre-2026-07-03 sprint behavior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_RunWalkDuration = 4.0f;

	/** Speed (UU/s) at which the run clip's stride looks natural — the loop plays at
	 *  AZP_RunSpeed / this. Tune until foot skate is gone (same pattern as AZP_AnimWalkRefSpeed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_RunAnimRefSpeed = 350.f;

	/** WORLD-space head stabilization during run bursts, 0..1. Freezing the head's track (the
	 *  arms treatment) only killed its LOCAL motion — it still rides the torso's run-swing and
	 *  whips around (dev 2026-07-03: "head facing the player, not jerking around"). 1 = the
	 *  head holds its reference orientation in component space (steady gaze at the player it's
	 *  facing) while the body pounds underneath; 0 = off. Applied by ABP_Shambler's C++ parent
	 *  (UZP_ShamblerGrabPoseAnimInstance) post-evaluate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_RunHeadStabilize = 1.f;

	/** True while the sprint-chase is in a RUN burst (the anim instance's head-stabilize gate). */
	bool IsRunBurstActive() const { return bRunningChase && bRunBurstNow; }

	/** Run cycle, played as a slot loop over the walk BlendSpace for the whole sprint. Default =
	 *  A_Shambler_RunStiffArmsHead: the NAAT run body with arms AND head/neck frozen at the
	 *  AS_NAAT_Zombie_LL_Idle pose (bake_shambler_run_arms.py + bake_shambler_scream_run_fixes.py
	 *  — dev 2026-07-03: "the run looks goofy with the head swinging around"). Swap to
	 *  A_Shambler_RunStiffArms (head animated) or plain A_Shambler_Run (everything animated). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_RunAnim;

	/** Degrees/sec it pivots to FACE the player during combat (scream / chase / attack). Fast enough to
	 *  track you circling it, but not instant. Wander still turns via movement orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float AZP_CombatTurnRate = 300.f;

	// --- Wander ("shambly" — short bursts, frequent stops, mid-leg stumbles) ---
	/** How far each wander LEG walks (UU). Smaller radius keeps the body in a tighter footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WanderRadius = 180.f;

	/** MoveToLocation acceptance radius (UU) for wander legs ('close enough' so it doesn't orbit
	 *  the exact point) — same 100.f repeated at the stumble-resume re-issue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WanderAcceptRadius = 100.f;

	/** Min straight-line distance (UU) a candidate wander point must be from the body's current
	 *  position. Without this the picker can land within 80 UU and the leg ends instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WanderMinLegDistance = 100.f;

	/** Target dot(forward, dirToCandidate) for picked legs. 1.0 = straight ahead (linear path),
	 *  0.0 = perpendicular (90° turn), -1.0 = behind. ~0.5 (~60° off forward) gives curving,
	 *  organic-looking wander paths instead of straight lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WanderTargetDot = 0.5f;

	/** Walk-phase duration (s). The Shambler walks for this many seconds (picking new legs as it
	 *  arrives at each dest), then drops to idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WalkDurationMin = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_WalkDurationMax = 6.0f;

	/** Idle-phase duration (s). The Shambler stops, plays the zombie idle animation, holds for this
	 *  many seconds, then resumes walking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_IdleDurationMin = 6.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_IdleDurationMax = 9.0f;

	/** Minimum seconds a wander leg must last before its "did I arrive?" check can fire. Without
	 *  this, when the AI rejects the picked path (unreachable) the body instantly "arrives"
	 *  (DistToDest never changes from start), the leg ends, a new pause begins — the rapid
	 *  pause→leg→pause cycle the dev called "firing off a bunch of decisions in a short period". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_MinLegDurationSec = 1.0f;

	/** Z offset (UU) applied to the mesh while the idle pose is showing. Compensates for the idle
	 *  anim's pelvis sitting higher than the walk's — without it, the feet visibly lift off the
	 *  ground at WALK→IDLE. Negative pulls the body down; tune until the foot plant matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_IdleMeshZOffset = -8.f;

	/** Blend-in time (s) for the idle slot anim when entering the IDLE phase. Long enough to cover
	 *  the CMC's natural deceleration from walking speed → 0. Shorter = jagged snap; longer =
	 *  walk pose leaks through the slot. 0.3 covers a default braking ramp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_IdleBlendInTime = 0.3f;

	/** Blend-out time (s) for the idle slot anim when leaving the IDLE phase. Long enough that the
	 *  BlendSpace below has time to ramp up from speed=0 to walking before the idle pose has fully
	 *  faded. 0.3 reads as a natural drift back into a walk cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_IdleBlendOutTime = 0.3f;

	/** Seconds after entering IDLE before the CMC is hard-locked to MOVE_None. During this delay
	 *  the CMC brakes from walk speed to ~0 naturally (smooth deceleration). After the delay,
	 *  MOVE_None freezes any residual nudges (RVO, leftover acceleration, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_IdleLockDelay = 0.35f;

	/** Per-second probability of interrupting an in-progress wander leg with a mid-leg stumble
	 *  (briefly stops moving, then resumes toward the same target). The dropping-then-resuming
	 *  motion is what reads as "zombie hesitation". 0 = no stumbles, 1 = stumble basically every leg.
	 *  Kept low so the wander stays smooth — high values stack pauses on top of leg-end pauses
	 *  and the body looks like it's stop-pivot-stop-pivot constantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_StumbleChancePerSec = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_StumbleMin = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_StumbleMax = 0.8f;

	// --- Audio ---
	/** Groan played exactly when the wander IDLE animation starts (one-shot per idle phase).
	 *  Lazily defaulted to /Game/Audio/Shambler/SFX_ZOMBIE_IDLE in BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	TObjectPtr<USoundBase> AZP_IdleSound;

	/** Footstep one-shots, random pick per step. Lazily filled in BeginPlay from
	 *  /Game/Audio/Shambler/Footsteps/SFX_SHAMBLER_FOOTSTEP_NN (sliced from the dev's
	 *  SFX_SHAMBLER_FOOTSTEPS reel — Scripts/Python/slice_footstep_reel.py). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	TArray<TObjectPtr<USoundBase>> AZP_FootstepSounds;

	/** Footstep VOLUME (dev 2026-07-03). 0 = silent feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_FootstepVolume = 1.f;

	/** Distance (UU) traveled per footstep. DISTANCE-based stepping is what cadence-matches
	 *  every gait automatically — wander 120 / chase 250 / run bursts 420, stumbles, pauses:
	 *  faster movement = proportionally faster steps, stationary = silent. Tune until the
	 *  sounds land on the visual foot plants. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_FootstepStride = 80.f;

	/** Random pitch spread per step (1 +/- this) so the steps don't sound mechanical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_FootstepPitchVar = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_LurkRange = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_LurkIntervalMin = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float AZP_LurkIntervalMax = 13.f;

	// --- Animations (defaulted to the retargeted Shambler clips) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_WalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_AttackLAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_AttackRAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_ScreamAnim;

	/** Directional death clips (retargeted FPP_Dag_Death1 = front fall, Death2 = back fall). Played
	 *  on death and held on the final frame as the corpse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_DeathFrontAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_DeathBackAnim;

	/** Directional hit-reaction clips (retargeted HitZombie_Front/Back). Played on non-lethal damage
	 *  — front hit when shot from the front, back hit when shot from behind. Mirrors the death
	 *  direction logic: same dot-product test against the actor's forward at the moment of impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_HitFrontAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AZP_HitBackAnim;

	/** Attacker-side NAAT grab clips (retargeted onto the necromorph skeleton — see
	 *  Scripts/Python/retarget_grab_anims.py). Paired 1:1 with the player's victim clips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabEntryAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabMunchAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabWrestleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabKickedAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabPushedAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Grab")
	TObjectPtr<UAnimSequence> AZP_GrabTakedownAnim;

	/** Speed (UU/s) at which the walk clip's stride looks natural. The clip's play rate scales with
	 *  actual speed / this, so the feet keep pace instead of sliding. Tune until skate is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_AnimWalkRefSpeed = 150.f;

	/** Mesh-Z nudge during the scream. With A_Shambler_ScreamPinned (lower body frozen at the
	 *  idle stance — dev 2026-07-03: the old clip lifted the feet a few inches), the legs ARE
	 *  the idle pose, so this matches AZP_IdleMeshZOffset (-8), not the old -16 whole-clip drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_ScreamMeshZOffset = -8.f;

	/** The death clips have no root motion, so the corpse lands floating at hip height. This lowers the
	 *  mesh by this much (UU) over the death clip so it settles on the ground. Tune until it grounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_DeathDropZ = 90.f;

	/** Finish the drop this many seconds BEFORE the clip ends, so it lands with the fall instead of
	 *  crawling the whole clip. The drop is also eased (fast then settle) to track the visual fall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_DeathDropLead = 0.5f;

	/** Eval period (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler")
	float AZP_EvalInterval = 0.25f;

	/** LEGACY — no longer read (BP_Shambler carries a stale serialized override of it, which is why
	 *  the replacement got a NEW name). See AZP_FlinchCooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_HitReactCooldown = 5.0f;

	/** Min seconds between flinch-CLIP plays (out of combat states) / swing hitches. The mesh
	 *  punch below is NOT gated — every single hit visibly jolts the body regardless. Purely
	 *  cosmetic: never pauses the AI, never cancels a swing; gameplay stagger stays block-only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_FlinchCooldown = 0.5f;

	/** Play-rate the in-flight swing drops to for AZP_SwingHitchTime when a hit lands mid-swing — a
	 *  near-freeze hit-stop ("absorbed the blow"). The swing still completes (hyper-armor holds);
	 *  it just visibly FEELS the hit. One hitch per swing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_SwingHitchRate = 0.05f;

	/** Seconds the hitch lasts before the swing snaps back to its phase rate (wind-up or strike). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_SwingHitchTime = 0.16f;

	/** Mesh jolt (UU) applied along the hit direction on EVERY landed hit, spring-decaying over
	 *  ~0.2 s. The always-on layer of hit feedback — reads in any state, even mid-swing, without
	 *  touching the AI or the playing animation. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_HitPunchStrength = 9.f;

	/** Punch spring-back speed (per second). Higher = snappier recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AZP_HitPunchRecovery = 12.f;

	/** Yaw deg/sec the body uses to pre-rotate toward the next wander dest during a pause. Picking
	 *  the next dest BEFORE the pause and turning during it means the next leg starts already
	 *  facing the right way — no awkward stand-still pivot the moment locomotion resumes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float AZP_PauseTurnRate = 90.f;

	UPROPERTY(BlueprintReadOnly, Category = "Shambler|State")
	EShamblerState State = EShamblerState::Wander;

	/** Play the flinch, cancel any in-flight swing, and pause AI for Duration. In the current design
	 *  this is the BLOCK reward path (player melee hits pass Duration 0 = no stagger — see
	 *  AZP_GraceCharacter AZP_HitStaggerDuration): it always reads, with no internal cooldown, because
	 *  it can only fire when one of the Shambler's own swings lands on a blocking player (~its swing
	 *  cadence) and the player-side AZP_BlockStaggerCooldown is the rate limit. */
	UFUNCTION(BlueprintCallable, Category = "Shambler|Combat")
	void ReceiveStaggerHit(float Duration);

	// IZP_Staggerable — forwards to ReceiveStaggerHit so the player can stagger this enemy
	// generically (block, and melee hits if AZP_HitStaggerDuration is ever raised above 0).
	virtual void ReceiveStagger_Implementation(float Duration) override { ReceiveStaggerHit(Duration); }

	// IZP_Revivable — death-state persistence + objective-driven revival (UZP_DeathSaveComponent).
	virtual void ApplyDeadStateInstant_Implementation() override;
	virtual void ReviveEnemy_Implementation() override;

	// IZP_Grabber — the victim's phase machine drives; we mirror each phase with the paired clip.
	virtual void OnVictimGrabPhase(EZP_GrabPhase NewPhase) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Lazily load the default Shambler anim clips for any slot the BP didn't override.
	 *  Done here (not the ctor) — ConstructorHelpers asset loading crashed on editor load. */
	void LoadAnimDefaults();
	void Evaluate();
	void SetState(EShamblerState NewState);
	void SetSpeed(float Speed);
	/** Play a stationary full-body one-shot (scream/swipe/flinch) on the AnimBP's DefaultSlot as a
	 *  dynamic montage. Returns the montage so callers can retime it mid-play (the swing wind-up
	 *  release). PlayRate < 1 slows the whole clip (used for the swing's telegraphed rear-back). */
	UAnimMontage* PlayOneShot(UAnimSequence* Anim, float PlayRate = 1.f);
	/** Play a clip on the AnimBP's DefaultSlot looped indefinitely (until StopSlotLoop). Used to
	 *  hold a real Idle pose during wander pauses — without it the BS_Shambler BlendSpace shows
	 *  walk-at-speed-0 while stopped, which reads as "marching in place" not "idle". Returns the
	 *  montage so callers (Tick vacuum-filler, sprint-chase) can track and release their own loop.
	 *  PlayRate: stride matching for moving loops (run = AZP_RunSpeed / AZP_RunAnimRefSpeed). */
	UAnimMontage* PlaySlotLoop(UAnimSequence* Anim, float PlayRate = 1.f);

	/** Exit the sprint (back to AZP_ChaseSpeed + walk BlendSpace). Safe to call when not running. */
	void StopRunChase();
	/** Stop whatever is currently on the DefaultSlot so locomotion takes back over. */
	void StopSlotLoop();

	/** COMBAT gap cover (dev 2026-07-03: "in combat, idle shouldn't happen at all"): instead of
	 *  vacuum-filling combat gaps with the docile wander idle, HOLD the final pose of whatever
	 *  combat clip just played — TickComponent pauses the active slot montage just before its
	 *  blend-out when the body is stationary in Scream/Chase/Attack. The next clip crossfades
	 *  over the held pose; movement or a state release resumes+ends it. No new animation content. */
	void ReleaseCombatPoseHold();

	/** Re-activate the locomotion AnimBP if a one-shot left the mesh in single-clip mode. */
	void EnsureLocomotion();

	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;             // clear sightline, walls block (same geometry)
	/** Pick a random navmesh point in AZP_WanderRadius and write it to WanderDest. Returns true if
	 *  found. Does NOT issue MoveToLocation — separated so we can pre-pick at pause-entry and
	 *  let the body rotate toward the new dest while standing idle. */
	bool PickNewWanderPoint();
	/** Issue MoveToLocation toward WanderDest. Call after pre-picking with PickNewWanderPoint. */
	void StartWanderLeg();
	/** Smoothly rotate the owner's yaw toward the current target at AZP_CombatTurnRate (per-frame). */
	void FaceTargetSmooth(float DeltaTime);
	void BeginAttack();
	void ApplyAttackDamage();

	/** In AZP_GrabRange + off cooldown: ask the victim (IZP_Grabbable) to be grabbed. Grabbed -> snap the
	 *  pair spacing/facing, State=Grab, play the entry clip. Deflected -> stagger + full cooldown. */
	void TryStartGrab();

	/** Restore movement/collision after a grab ends (any outcome) and pick the follow-up state. */
	void EndGrabOnShambler(bool bResumeChase);

	/** Shot/staggered mid-grab: the grab breaks — the victim is released with no outcome anim
	 *  (the victim's AbortGrab calls back OnVictimGrabPhase(None), which does our cleanup). */
	void BreakGrabFromDamage();

	/** Pause the AI (bStaggered) for Duration WITHOUT playing the flinch clip — used after grab
	 *  outcomes where a paired reaction clip already owns the slot. */
	void PauseAIWithoutFlinch(float Duration);

	/** Queue the idle slot loop for the moment a one-shot clip blends out while the AI is still
	 *  paused. NOW LOOM-ONLY (FailKnockdown — dev spec 2026-07-03: "it LOOMS: idle in place until
	 *  they're back up"): every other combat gap uses the Tick pose-hold instead, because the
	 *  docile idle is banned in combat (dev 2026-07-03). Only engages if still bStaggered when
	 *  the clip ends; the wait-for-victim-up release stops the loop as locomotion resumes. */
	void ScheduleGrabIdleFill(float ReactionClipLen);

	/** Knocked the victim down: hold in place (idle loop) until IZP_Grabbable::IsGrabRecovering
	 *  goes false — no pathing circles around a downed body — then resume the chase. MinWait
	 *  protects the swipe reaction; a hard failsafe releases after ~12s no matter what. */
	void BeginWaitForVictimUp(float MinWait);

	/** After a grab ends, the two capsules may still INTERPENETRATE (AZP_GrabPairDistance < the sum
	 *  of their radii) — restoring collision immediately makes the engine depenetrate them: the
	 *  random shove/slide/orbit the dev reported. Keep ignoring the victim until the capsules
	 *  are actually clear (polled). NEVER force-restores while overlapped short of the 15s
	 *  abandon point (the old 4s failsafe fired MID-LOOM and shoved the zombie away). */
	void DeferCollisionRestore();

	/** Continuous 6s post-release motion trace, 0.1s samples ([GrabSlideProbe] log lines). */
	void StartGrabSlideProbe();

	/** Fires AZP_EscapePushbackDelay seconds into a kick/push escape: captures the from/to points and
	 *  arms the TickComponent slide-to-stop (bEscapePushback). */
	void StartEscapePushback();

	/** Wind-up over — snap the in-flight swing montage from AZP_WindupPlayRate to AZP_StrikePlayRate. */
	UFUNCTION()
	void ReleaseSwing();

	/** A block/stagger landed mid-swing: kill the queued hit + release timers so a staggered swing
	 *  never invisibly deals its damage. */
	void CancelPendingSwing();

	/** Absorb-hitch: briefly drop the in-flight swing to AZP_SwingHitchRate so a mid-swing hit visibly
	 *  registers without cancelling the swing. Capped at one per swing. Returns true only if the
	 *  hitch actually fired (callers only consume the flinch cooldown on a real reaction). */
	bool DoSwingHitch();

	/** Hitch over — restore the swing to the rate of its current phase (wind-up or strike). */
	UFUNCTION()
	void RestoreSwingRate();

	UFUNCTION()
	void OnOwnerDied();

	/** Bound to the owner's OnTakePointDamage — applies head/body damage to the health component. */
	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir, const UDamageType* DamageType, AActor* DamageCauser);

	/** PROBE: did ANY damage event reach the Shambler actor at all? */
	UFUNCTION()
	void OnAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	/** Bound to AIController::ReceiveMoveCompleted — instantly chains the next wander leg when a move
	 *  ends (arrived/aborted). Without this the body stops at the acceptance radius and waits up to
	 *  AZP_EvalInterval (0.25s) for the next Evaluate to fire — that gap is what reads as "still pausing". */
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	/** Delayed-by-AZP_IdleLockDelay callback: once the CMC has braked from walk speed to ~0, lock the
	 *  movement mode to MOVE_None and zero velocity. Lets the deceleration play out visibly under
	 *  the slot blend instead of snapping. */
	UFUNCTION()
	void LockIdleMovement();

	/** PROBE: trace a Visibility ray straight through the mesh center to prove whether its collision is hittable. */
	void ProbeShootable();

	/** Play one random footstep one-shot at the body (AZP_FootstepVolume / AZP_FootstepPitchVar). */
	void PlayFootstep();

	void UpdateLurk(float DistToPlayer);

	UPROPERTY() TObjectPtr<ACharacter> Owner;
	UPROPERTY() TObjectPtr<AAIController> AICon;
	UPROPERTY() TObjectPtr<UAudioComponent> GrabLoopAudio; // live grapple snarl loop (stopped on grab end)
	UPROPERTY() TObjectPtr<UZP_EnemyAudioComponent> Audio;
	UPROPERTY() TObjectPtr<UZP_HealthComponent> Health;
	UPROPERTY() TObjectPtr<AActor> Target;
	float MeshBaseRelZ = 0.f;
	FVector2D MeshBaseRelXY = FVector2D::ZeroVector; // rest XY of the mesh — punch offsets from here
	FVector2D MeshPunch = FVector2D::ZeroVector;     // live hit-jolt offset (actor-local XY), decays in Tick
	FVector SpawnLocation = FVector::ZeroVector; // cached at BeginPlay; wander stays tethered around this

	FTimerHandle EvalTimer;
	FTimerHandle AttackHitTimer;
	FTimerHandle WindupReleaseTimer; // fires at the end of the slowed rear-back -> AZP_StrikePlayRate
	FTimerHandle HitchRestoreTimer;  // ends the absorb-hitch -> phase play rate
	FTimerHandle ProbeTimer;
	FTimerHandle IdleLockTimer; // delays MOVE_None until braking deceleration finishes (smooth WALK→IDLE)
	FTimerHandle StaggerHandle;
	FTimerHandle GrabIdleFillTimer; // fills the post-grab-reaction gap with the idle loop (anti "swim")
	FTimerHandle VictimUpWaitTimer; // holds the AI idle until the knocked-down victim is back up
	FTimerHandle CollisionRestoreTimer; // restores pawn-vs-pawn collision once the capsules are clear
	FTimerHandle GrabSlideProbeTimer;   // continuous post-release motion trace ([GrabSlideProbe])
	FTimerHandle LatchWindowProbeTimer; // [LatchProbe] 2s post-LATCH window ticker (0.1s samples)
	FVector LatchWindowOrigin = FVector::ZeroVector; // shambler position at the latch
	FTimerHandle EscapePushbackDelayTimer; // holds the pair spacing through the push-contact beat
	FVector SlideProbeOrigin = FVector::ZeroVector; // shambler position at the moment of release
	double SlideProbeStart = 0.0;

	// live escape pushback (driven in TickComponent — see StartEscapePushback)
	bool bEscapePushback = false;
	double EscapePushbackStart = 0.0;
	FVector EscapePushbackFrom = FVector::ZeroVector;
	FVector EscapePushbackTo = FVector::ZeroVector;

	// latch lunge-in (driven in TickComponent — replaces the one-frame pair-spacing teleport)
	bool bGrabSnapIn = false;
	double GrabSnapStart = 0.0;
	FVector GrabSnapFrom = FVector::ZeroVector;
	FVector GrabSnapTo = FVector::ZeroVector;

	/** True while Chase is holding position inside AZP_AttackRange (stand ready, face the player —
	 *  no MoveToActor re-paths orbiting the target). */
	bool bChaseHoldingInRange = false;
	bool bStaggered = false;

	float LostSightTimer = 0.f;
	bool bWanderMoving = false;
	bool bStumbling = false;
	double StumbleEndTime = 0.0;
	double LastHitReactTime = -1000.0;
	float IdleDuration = 1.f;  // randomized 6-9s at idle-entry
	float WalkDuration = 4.f;  // randomized 3-6s at walk-entry
	FVector WanderDest = FVector::ZeroVector;
	double LastAttackTime = -1000.0;
	double LastGrabTime = -1000.0;   // grab cooldown anchor (set on landed AND deflected grabs)
	double LastLatchTime = -1000.0;  // [LatchProbe] moment the last grab actually LATCHED — probes log dt vs this
	bool bAttackIsLeft = false;
	float CurrentSwingTotalTime = 0.f;                 // real seconds this swing lasts (rate-remapped)
	float CurrentSwingHitTime = 0.f;                   // real seconds until this swing's damage sweep fires
	TWeakObjectPtr<UAnimMontage> ActiveSwingMontage;   // in-flight swing, retimed at wind-up release
	TWeakObjectPtr<UAnimMontage> SlotVacuumFill;       // Tick-spawned idle fill (WANDER ONLY — never in combat)
	TWeakObjectPtr<UAnimMontage> CombatPoseHold;       // combat clip paused at its final pose to cover a gap
	TWeakObjectPtr<UAnimMontage> RunLoopMontage;       // sprint-chase run cycle (slot loop)
	bool bRunningChase = false;                        // Chase is in the sprint band (> AZP_RunTriggerDistance)
	bool bRunBurstNow = false;                         // sprint sub-phase: true = run burst, false = fast walk
	double RunPhaseStart = 0.0;                        // when the current burst/walk sub-phase began
	bool bSwingReleased = false;                       // strike phase reached (RestoreSwingRate picks the right rate)
	bool bHitchedThisSwing = false;                    // absorb-hitch fired for the current swing
	bool bLastHitFront = true; // which side the last damage came from -> death-fall direction
	float CurrentScreamHold = 2.f; // per-aggro scream hold: AZP_ScreamHoldTime on sight, AZP_HurtScreamHoldTime on damage
	double DeathStartTime = 0.0;
	float DeathAnimLen = 0.f;
	bool bDropping = false;
	float StateTimer = 0.f;
	bool bDead = false;

	// distance-based footsteps (accumulates 2D travel; one step per AZP_FootstepStride)
	float StepDistanceAccum = 0.f;

	// lurk audio
	float LurkTimer = 0.f;
	float LurkInterval = 8.f;
	bool bLurkInit = false;
	bool bWasInLurkRange = false;
};
