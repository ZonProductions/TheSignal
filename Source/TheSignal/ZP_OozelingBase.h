// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_OozelingBase
 *
 * Purpose: The Oozeling (BigBlob pack) — wall-crawling stalker enemy. Ping-pongs along an
 *          assigned AZP_OozeClimbPath / AZP_ScytheerClimbPath spline using the Scytheer's proven
 *          wall-climb technique: CharacterMovement parked in MOVE_None while the body is
 *          driven per tick by SetActorLocationAndRotation from the spline location, tangent,
 *          and lerped per-point wall normal (MakeOrientation). No path assigned = random
 *          navmesh wander around spawn with idle pauses.
 *
 *          THE ATTACK — TOUCH BURST (2026-07-13, dev design): it does not swing. It closes to
 *          contact, LATCHES, and starts ERUPTING: a fuse of AZP_TouchFuseTime seconds
 *          (vibrating telegraph clip). COUNTERPLAY: kill it during the fuse = clean death, NO
 *          burst; get outside AZP_BurstRadius before the fuse ends = it pops without coating
 *          you. If the fuse elapses with the player in radius, it SPLATS ITSELF: Death2 plays
 *          (the burst IS its death — normal death flow, corpse persists) and the victim takes
 *          dissolving-ooze damage over time (AZP_TouchDamagePercentPerSecond% of max HP per
 *          tick for AZP_TouchDamageDuration seconds; defaults 5%/s x 5s = ~25%). It cannot be
 *          stagger-interrupted while erupting — only death defuses it. One Oozeling = one burst.
 *
 *          AGGRO + CHASE (2026-07-13): player within AZP_DetectionRange + two-way LOS
 *          (+ navmesh reachability when
 *          grounded — closed doors block aggro; skipped while wall/ceiling-clinging, the body
 *          isn't on the navmesh up there). Routing on aggro: CEILING (path normal pointing
 *          down) -> Drop: peels off instantly, free-falls, chases on landing. WALL -> WallDescent:
 *          sprints down the spline toward its GROUND END (the lower endpoint, found automatically
 *          — paths work authored in either direction), leaves the spline the moment it reaches
 *          floor height, then chases. GROUNDED -> straight to Chase (navmesh MoveToActor, run clip,
 *          stuck watchdog). De-aggro (lost sight AZP_LoseSightTime / beyond AZP_GiveUpRange /
 *          stuck) -> walks back to the nearest spline point (ReturnToPatrol), resumes patrol.
 *
 *          DEATH: death SFX fires at the kill moment; if the body isn't on the ground
 *          (wall/ceiling/mid-air) it levels out and FREE-FALLS first — capsule stops blocking
 *          the player and bullets but still lands on world geometry — and only on landing does
 *          the corpse finalize and play AZP_DieAnim (Death2), final frame held. Grounded kills
 *          play it immediately.
 *
 *          Animation: whole BigBlob AnimSequence clips (walk/run/idle/fall/hit/die) on a
 *          SingleNode mesh — NOT the Scytheer's frame-slicing (the BigBlob pack ships separate
 *          clips). Defaults lazy-load from /Game/BigBlob/Animations/ in BeginPlay; never
 *          ConstructorHelpers, never hard CDO asset refs (DEAD ENDS 2026-06-29).
 *
 * Owner Subsystem: EnemyAI / Oozeling
 *
 * Blueprint Extension Points:
 *   - Mesh: SkeletalMesh asset (SKM_BigBlob) set in the child BP; inherited Character Mesh.
 *   - AZP_WalkAnim / AZP_RunAnim / AZP_IdleAnim / AZP_FallAnim / AZP_HitAnim / AZP_DieAnim.
 *   - SFX slots: silent until real Oozeling audio exists — swap in editor.
 *
 * Dependencies: UZP_HealthComponent, AZP_ScytheerClimbPath (patrol spline API),
 *               UZP_SFXStatics, AAIController, NavigationSystem.
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZP_Staggerable.h"
#include "ZP_Revivable.h"
#include "ZP_OozelingBase.generated.h"

class UAnimSequence;
class UZP_HealthComponent;
class UDamageType;
class UPrimitiveComponent;
class USoundBase;
class AController;
class AAIController;
class AZP_ScytheerClimbPath;

UENUM(BlueprintType)
enum class EOozelingState : uint8
{
	Wander,         // patrolling the spline (wall-climb) or random navmesh roam
	WallDescent,    // aggroed on a WALL — sprints down the spline toward its ground end before chasing
	Drop,           // aggroed on a CEILING (or descent ended elevated) — free-falls, chases on landing
	Chase,          // running at the player — contact (AZP_TouchRange) starts the eruption fuse
	Erupt,          // latched on, fuse burning (AZP_TouchFuseTime) — kill it NOW to defuse the burst
	Hit,            // shot / staggered — brief flinch, holds position (stays glued to the wall)
	DieFalling,     // dead while off the ground — free-falls; corpse finalizes on landing
	Die,            // dead — holds the death clip's final frame as corpse
	ReturnToPatrol  // de-aggroed; walking back to the nearest spline point before resuming patrol
};

UCLASS()
class THESIGNAL_API AZP_OozelingBase : public ACharacter, public IZP_Staggerable, public IZP_Revivable
{
	GENERATED_BODY()

public:
	AZP_OozelingBase();

	// IZP_Staggerable — flinch and hold (AI paused) for Duration. Player melee hit / block.
	virtual void ReceiveStagger_Implementation(float Duration) override;

	// IZP_Revivable — death-state persistence + objective-driven revival (UZP_DeathSaveComponent).
	virtual void ApplyDeadStateInstant_Implementation() override;
	virtual void ReviveEnemy_Implementation() override;

	// ── Detection ───────────────────────────────────────────────────
	/** Straight-line distance to consider the player for aggro. Grounded aggro is additionally
	 *  gated by two-way LOS + navmesh reachability (closed doors block it); wall/ceiling aggro
	 *  is gated by LOS only — the clinging body isn't on the navmesh, so a reachability check
	 *  from up there would always fail and mute aggro entirely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_DetectionRange = 1000.f;

	/** Seconds without line of sight before a chasing Oozeling de-aggros. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_LoseSightTime = 4.f;

	/** Distance beyond which a chasing Oozeling abandons pursuit regardless of sight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_GiveUpRange = 2200.f;

	/** Max navmesh PATH length (UU) that counts as "same geometry" for grounded aggro. A closed
	 *  door blocks the navmesh, so even a short straight-line gap reads as unreachable -> no aggro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_MaxReachablePathLength = 1600.f;

	/** Height above the actor pivot the LOS trace starts from (the Oozeling's 'eye'). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_LOSEyeZOffset = 30.f;

	/** Height above the target's pivot the LOS trace aims at (roughly chest height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_LOSTargetZOffset = 40.f;

	/** Z above the patrol path's GROUND END (the lower of its two endpoints — found
	 *  automatically, so paths can be authored in either direction) that counts as "on the
	 *  wall" — routes aggro through WallDescent instead of straight Chase. Half this value is
	 *  the descent's "reached the floor" arrival band. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_OnWallZThreshold = 100.f;

	/** Ceiling classification: if the path's wall normal at the current spline distance has
	 *  Z <= this (pointing downward), the Oozeling counts as ceiling-clinging and aggro DROPS
	 *  it instead of running the spline down. -0.35 tolerates sloped ceilings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Detect")
	float AZP_CeilingNormalZ = -0.35f;

	// ── Patrol (the wall-climb route — assign an AZP_OozeClimbPath) ─────
	/** The spline route to ping-pong along, wall-climbing via its per-point wall normals.
	 *  Typed as the shared base so an AZP_OozeClimbPath (preferred) or any climb path works.
	 *  While set, CharacterMovement is parked in MOVE_None and the body is spline-driven.
	 *  NOTE: per-instance object refs like this are WIPED by C++ rebuild reinstancing —
	 *  re-check placed Oozelings after a rebuild. Unset = random navmesh wander. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Patrol")
	TObjectPtr<AZP_ScytheerClimbPath> AZP_PatrolPath;

	/** SELF-HEAL: if AZP_PatrolPath is empty at BeginPlay, bind to the nearest climb path
	 *  within AZP_AutoBindPathRadius. Exists because C++ rebuild reinstancing is known to wipe
	 *  per-instance object refs (KB 20260617b) — a rebuild must never leave a placed Oozeling
	 *  standing dumb. An explicitly assigned AZP_PatrolPath always wins; set false only for an
	 *  Oozeling that must NOT patrol a nearby path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Patrol")
	bool bAZP_AutoBindPath = true;

	/** Search radius (UU) for the BeginPlay auto-bind above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Patrol")
	float AZP_AutoBindPathRadius = 3000.f;

	/** Max rotational speed (deg/s) while patrolling. Caps the endpoint 180° reversal
	 *  (~0.5s at 360) so it never snaps in one frame; normal spline curves track invisibly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Patrol")
	float AZP_PatrolTurnRate = 360.f;

	// ── Movement ────────────────────────────────────────────────────
	/** Crawl speed (UU/s) along the patrol spline AND for the navmesh wander fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_WanderSpeed = 60.f;

	/** Run speed (UU/s) for the Chase state AND the WallDescent sprint down the spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_ChaseSpeed = 280.f;

	/** MoveToActor acceptance radius while chasing. MUST stay below AZP_TouchRange or the chase
	 *  completes outside contact and the burst never triggers — default 90 drives the body right
	 *  into the player (capsules physically meet at ~90). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_ChaseAcceptanceRadius = 90.f;

	/** Max yaw speed (deg/s) used to track the player while planted at crowding range —
	 *  keeps a jammed/cornered body visibly stalking instead of freezing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_CombatTurnRate = 300.f;

	/** Movement (UU) below which the chase body counts as 'not moving' for stuck detection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_ChaseStuckMoveThreshold = 30.f;

	/** Seconds stuck during Chase before retrying MoveToActor with a wider acceptance radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_ChaseStuckRepathTime = 2.f;

	/** Seconds of not moving during Chase before de-aggro back to patrol (anti door-jam watchdog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Move")
	float AZP_ChaseStuckGiveUpTime = 4.f;

	// ── Touch burst (THE attack — it kills itself to hurt you) ─────
	/** Contact distance (UU, center-to-center) that triggers the burst during Chase, gated on
	 *  two-way LOS so it can't fire through a thin wall. The two capsules physically meet at
	 *  ~110 (Oozeling 55 + player 55); default gives a ~30 UU grace shell. MUST stay above
	 *  AZP_ChaseAcceptanceRadius or the chase completes outside contact and never bursts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_TouchRange = 140.f;

	/** THE TOUCH-TO-DAMAGE TIME: seconds between latching on (contact) and the burst. This is
	 *  the melee counterplay window — kill it before the fuse ends and there is NO burst/DoT.
	 *  0 = instant eruption (the pre-fuse behavior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_TouchFuseTime = 1.0f;

	/** Radius (UU) the victim must still be inside at DETONATION for the ooze DoT to land —
	 *  dodging/sprinting out during the fuse makes it pop harmlessly (it still dies). Keep
	 *  comfortably above AZP_TouchRange so standing still never escapes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_BurstRadius = 250.f;

	/** Percent of the VICTIM'S max health dealt per DoT tick after the burst (5 = 5%/tick). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_TouchDamagePercentPerSecond = 5.f;

	/** Total seconds the dissolving-ooze DoT lasts (with the default 1s interval and 5%/tick,
	 *  5 seconds = ~25% of the player's max health). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_TouchDamageDuration = 5.f;

	/** Seconds between DoT ticks. First tick lands at the touch itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Attack")
	float AZP_TouchDamageInterval = 1.f;

	// ── Wander (random-roam fallback when AZP_PatrolPath is unset) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Wander")
	float AZP_WanderRadius = 600.f;

	/** 2D distance to the wander destination that counts as arrival (pause-then-repick). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Wander")
	float AZP_WanderArriveRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Wander")
	float AZP_PauseMin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Wander")
	float AZP_PauseMax = 3.5f;

	/** Acceptance/arrival radius for wander + return-to-patrol MoveToLocation calls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Wander")
	float AZP_MoveAcceptanceRadius = 80.f;

	// ── Falling (ceiling drop + death fall) ─────────────────────────
	/** Deg/s the falling body rights itself toward upright (a wall/ceiling frame can start it
	 *  sideways or inverted; 540 covers a full flip in ~0.33s, well inside a room-height fall). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Fall")
	float AZP_FallUprightRate = 540.f;

	/** Safety: max seconds a fall (Drop or DieFalling) may last before it resolves where it is
	 *  (chase / corpse-finalize) — covers a body that never gets a Landed event (pit, geo hole). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Fall")
	float AZP_FallTimeout = 5.f;

	// ── Damage / Death ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Damage")
	float AZP_MaxHealth = 100.f;

	/** Flat per-bullet damage — the blob is one uniform body, no headshot zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Damage")
	float AZP_BodyShotDamage = 20.f;

	/** Min seconds between hit-react flinches so sustained fire can't perma-stunlock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Damage")
	float AZP_HitReactCooldown = 1.0f;

	// ── Animation clips (whole BigBlob sequences, SingleNode playback) ──
	/** Looping crawl clip. Lazy default: /Game/BigBlob/Animations/Walk_Roll/AS_BigBlob_Walk_FW. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_WalkAnim;

	/** Looping run clip (Chase + WallDescent). Lazy default: AS_BigBlob_Run_FW. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_RunAnim;

	/** Looping idle clip (wander pauses). Lazy default: AS_BigBlob_Idle_Breath. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_IdleAnim;

	/** Looping mid-air clip for the ALIVE ceiling drop. Lazy default: AS_BigBlob_Jump_Loop.
	 *  (The death fall instead freezes the pose it died in.) Empty = keep the walk clip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_FallAnim;

	/** Eruption-fuse telegraph clip (latched on, about to blow), looped for long fuses.
	 *  Default: AS_BigBlob_Hug_Attack (dev pick 2026-07-13 — it hugs you before it blows).
	 *  Empty = keeps the idle clip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_EruptAnim;

	/** CUTOFF (seconds into AZP_EruptAnim): freeze the erupt clip at this time and hold that
	 *  pose for the rest of the fuse — clips the clip's tail (e.g. keep Hug_Attack's vertical
	 *  rise, cut the flubber thrash after it). 0 = play the full clip (looping). Scrub the
	 *  clip in the anim editor to find the freeze time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	float AZP_EruptAnimCutoff = 0.f;

	/** One-shot flinch clip. Lazy default: AS_BigBlob_GetHit. Empty = no flinch (damage still applies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_HitAnim;

	/** One-shot death clip, final frame held as the corpse pose — plays AFTER the body reaches
	 *  the ground (death fall). Lazy default: AS_BigBlob_Death2 (Update1 set).
	 *  Empty = freeze whatever pose was playing at the kill. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	TObjectPtr<UAnimSequence> AZP_DieAnim;

	/** Play-rate on the walk clip so the crawl cadence can be matched to AZP_WanderSpeed
	 *  without re-authoring the clip (the pack clip is ~0.97s per cycle). Live-tunable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	float AZP_WalkPlayRate = 1.0f;

	/** Play-rate on the run clip (Chase / WallDescent cadence vs AZP_ChaseSpeed). Live-tunable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Anim")
	float AZP_RunPlayRate = 1.0f;

	// ── Audio (all optional — silent until real Oozeling audio is dropped in) ──
	/** One-shot at the moment of aggro (Far carry). Lazy default: /Game/Audio/Oozeling/SFX_Oozeling_Alert. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_AlertSound;

	/** One-shot when the eruption fuse starts (the "kill it NOW" audio telegraph, Far carry).
	 *  Lazy default: /Game/Audio/Oozeling/SFX_Oozeling_Erupt. Silent until the asset exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_EruptSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_HitSound;

	/** Death cry when KILLED (shot/melee — incl. a fuse defuse). Fires at the kill moment
	 *  (before any death fall), not on corpse landing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_DeathSound;

	/** TOUCH-DEATH sound: the burst/splat when its own eruption kills it (replaces
	 *  AZP_DeathSound for that death only). Lazy default: /Game/Audio/Oozeling/SFX_Oozeling_Burst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_BurstSound;

	/** Movement squelch one-shot, played once per AZP_FootstepStride of 2D travel. Distance is
	 *  measured from the body's OWN position delta (not GetVelocity()) so it cadence-matches the
	 *  teleport-driven spline patrol, where velocity reads 0. 0 volume / empty slot = silent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	TObjectPtr<USoundBase> AZP_FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	float AZP_FootstepVolume = 1.f;

	/** Distance (UU) of 2D travel per squelch — cadence scales with actual speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	float AZP_FootstepStride = 80.f;

	/** Random pitch spread per squelch (1 +/- this) so one asset never reads as a loop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oozeling|Audio")
	float AZP_FootstepPitchVar = 0.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Oozeling|State")
	EOozelingState State = EOozelingState::Wander;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	/** CMC Falling->grounded transition. Finalizes a death fall into the corpse (plays the death
	 *  clip) and turns an alive ceiling Drop into Chase. */
	virtual void Landed(const FHitResult& Hit) override;

private:
	bool bDead = false;
	bool bAggro = false;
	/** True only while Detonate()'s self-kill runs — routes OnOwnerDied to AZP_BurstSound
	 *  (touch-death) instead of AZP_DeathSound (killed). */
	bool bBurstDeath = false;
	float LostSightTimer = 0.f;
	float PauseTimer = 0.f;
	bool bWanderMoving = false;
	FVector WanderDest = FVector::ZeroVector;
	float PatrolDistance = 0.f;
	int32 PatrolDir = 1;
	double LastHitReactTime = -1000.0;
	/** While true, Hit holds (AI paused) until StaggerHandle fires instead of auto-returning
	 *  when the flinch clip ends. Set by ReceiveStagger. */
	bool bStaggerHold = false;
	FTimerHandle StaggerHandle;
	float ChaseStuckTimer = 0.f;
	FVector LastChaseStuckLoc = FVector::ZeroVector;
	/** World time the current fall state (Drop / DieFalling) began — drives AZP_FallTimeout. */
	double FallStateStartTime = 0.0;
	/** Touch-burst DoT bookkeeping — runs on the corpse (the actor persists after the burst). */
	FTimerHandle TouchDoTHandle;
	int32 TouchDoTTicksRemaining = 0;
	TWeakObjectPtr<APawn> TouchDoTVictim;
	/** World time the eruption fuse started (Erupt entry) — drives AZP_TouchFuseTime. */
	double EruptStartTime = 0.0;
	/** The last DESIRED patrol up (MakeOrientation output), seeded from the path's authored
	 *  normal at every spline rejoin. This — never the body's physical up — is the continuity
	 *  reference: a spawn/revive/rejoin body is upright, which projects to zero on vertical
	 *  climbs and used to poison the anti-flip logic into locking the wrong wall side. */
	FVector LastPatrolUp = FVector::UpVector;

	// whole-clip SingleNode playback state
	UPROPERTY() TObjectPtr<UAnimSequence> CurrentClip;
	bool bClipLoops = true;
	bool bClipCompleteFired = false;
	float CurrentClipLen = 0.f;
	bool bWarnedNullClip = false;       // once-per-instance null-clip warning fired
	bool bWarnedDegeneratePath = false; // once-per-instance zero-length-path warning fired

	// distance-based movement squelch: accumulate 2D position delta, one squelch per stride.
	float StepDistanceAccum = 0.f;
	FVector LastFootstepLoc = FVector::ZeroVector;

	UPROPERTY() TObjectPtr<UZP_HealthComponent> Health;
	UPROPERTY() TObjectPtr<AAIController> AICon;

	void EnterState(EOozelingState NewState);
	/** Route an aggro (or a post-flinch resume while aggroed) by current surface situation:
	 *  mid-air -> Drop, ceiling-clinging -> Drop, wall-clinging -> WallDescent, else Chase. */
	void EnterChaseRouted();
	/** Spline distance of the patrol path's GROUND end — whichever endpoint (0 or full length)
	 *  sits lower in Z. Descent target + elevation reference; makes path direction irrelevant. */
	float GetGroundEndDistance() const;
	/** Corpse endpoint: collision fully off, movement disabled, Die state (plays AZP_DieAnim). */
	void FinalizeCorpse();
	/** Fuse end: the Oozeling splats itself. Arms the ooze DoT on the player ONLY if they are
	 *  still within AZP_BurstRadius with LOS (dodge counterplay), then dies through the normal
	 *  death flow (SFX at the burst moment, Death2, corpse persists via DeathSave). Killing it
	 *  before this runs (during the fuse) skips the burst entirely — the melee counterplay. */
	void Detonate();
	/** One DoT tick: AZP_TouchDamagePercentPerSecond% of the victim's max health via the standard
	 *  ApplyDamage pipeline (the player's own i-frames/gates apply). Timer-driven on the corpse. */
	void TickTouchDoT();
	/** Shared per-tick fall handling (Drop + DieFalling): rights the body toward upright at
	 *  AZP_FallUprightRate. Returns true when AZP_FallTimeout has elapsed. */
	bool TickFallingUpright(float DeltaTime);
	/** Whole-clip SingleNode playback. Re-asserts playing state + play rate when the clip is
	 *  already current (live-tunable rates, revive-after-freeze). bForceRestart replays even if
	 *  Clip is already current (one-shot flinch spam). No-op with a one-time warning if null. */
	void PlayClip(UAnimSequence* Clip, bool bLoop, float Rate = 1.f, bool bForceRestart = false);
	/** Detects one-shot clip completion (SingleNode non-looping playback) and fires OnClipComplete once. */
	void TickAnim();
	void OnClipComplete();
	/** Fill any anim/SFX slot the Blueprint hasn't overridden. BeginPlay-lazy LoadObject —
	 *  ConstructorHelpers /Game loads during CDO construction crash the editor (Shambler lesson). */
	void LoadAssetDefaults();
	void PlayFootstep();
	void PickNewWanderPoint();
	void SetMaxWalkSpeed(float Speed);
	/** Yaw-only smooth tracking of the player at AZP_CombatTurnRate — used while planted at
	 *  crowding range so the body visibly stalks instead of freezing (Scytheer pattern). */
	void FaceTargetSmooth(float DeltaTime);
	APawn* GetPlayer() const;
	/** Two-way LOS on ECC_WorldStatic, skipping QueryOnly trigger volumes both directions —
	 *  complex-as-simple walls have no backfaces, so the reverse trace catches the front face
	 *  (Scytheer/Shambler through-wall aggro fix). */
	bool HasLOS(const AActor* Target) const;
	/** Grounded-aggro gate: same connected navmesh region within AZP_MaxReachablePathLength?
	 *  Closed doors block the navmesh, so this returns false through any solid barrier. */
	bool IsPlayerReachable(const AActor* Target) const;

	UFUNCTION()
	void OnOwnerDied();

	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir, const UDamageType* DamageType, AActor* DamageCauser);

	/** Compose a rotator whose +X points along Forward and +Z along Up (orthonormalized).
	 *  ReferenceUp = the body's CURRENT up, used for continuity when the authored normal runs
	 *  nearly parallel to the tangent (steep wall segments with up-ish normals): the projected
	 *  residual there is numerically erratic and used to snap the body 180° mid-wall ("bottom
	 *  vs top confused", dev report 2026-07-14). Strong authored normals (floor/wall/ceiling
	 *  proper) are always trusted as-is, so deliberate ceiling crawls still invert correctly. */
	static FRotator MakeOrientation(const FVector& Forward, const FVector& Up, const FVector& ReferenceUp);
};
