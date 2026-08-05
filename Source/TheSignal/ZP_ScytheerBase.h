// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ScytheerBase
 *
 * Purpose: Ground-roaming Scytheer enemy. Wanders the navmesh at a slow pace; when the player
 *          enters AZP_DetectionRange with line of sight, transitions to Alert (plays idle pose +
 *          alert SFX for AZP_AlertHoldTime), then Chase (runs toward player). On reach, swings one
 *          of three attack variants and applies damage at the clip midpoint. Takes bullets
 *          (capsule blocks Visibility — same pattern as the Shambler). 5 body shots = dead.
 *
 *          Animation is sliced from a single source AnimSequence using frame ranges (walk 1-33,
 *          run 35-53, idle 55-190, attack 1/2/3, hit 323-353, die 354-500) via UAnimSingleNodeInstance.
 *          No AnimGraph required.
 *
 *          AZP_ScytheerClimbPath (the spline-based wall walker variant) is preserved in the repo
 *          but no longer driven by this base — kept for a possible wall-walking subclass later.
 *
 * Owner Subsystem: EnemyAI / Scytheer
 *
 * Blueprint Extension Points:
 *   - Mesh: SkeletalMesh asset set in the child BP. The inherited Character Mesh is used.
 *   - AZP_SingleAnim: the source clip to slice.
 *   - SFX properties: all default to Crawler/Shambler placeholders — swap in editor for real audio.
 *
 * Dependencies: UZP_HealthComponent, AAIController, NavigationSystem.
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZP_Staggerable.h"
#include "ZP_Revivable.h"
#include "ZP_ScytheerBase.generated.h"

class UAnimSequence;
class UAnimSingleNodeInstance;
class UZP_HealthComponent;
class UDamageType;
class UPrimitiveComponent;
class USoundBase;
class USoundAttenuation;
class AController;
class AAIController;
class AZP_ScytheerClimbPath;

UENUM(BlueprintType)
enum class EScytheerState : uint8
{
	Wander,         // roaming the navmesh slowly
	Alert,          // just noticed the player — holds idle pose + plays alert SFX (GROUND aggro)
	WallDescent,    // aggroed while clinging to a wall — runs DOWN the spline before chasing
	Chase,          // running toward the player on the ground
	Attack,         // mid-swipe
	Hit,            // shot — brief flinch
	Die,            // dead, holds final frame as corpse
	ReturnToPatrol  // de-aggroed; walking back to the nearest patrol spline point before resuming Wander
};

/** Which of the two melee attacks the Scytheer is committing to. */
UENUM(BlueprintType)
enum class EScytheerAttackType : uint8
{
	Swipe,   // base strike, used when the player is already within melee (AZP_AttackRange)
	Lunge    // gap-closer, used when the player is beyond melee but within AZP_LungeRange — drives
	         // the body forward to close the distance so the player can't kite / stand out of reach
};

UCLASS()
class THESIGNAL_API AZP_ScytheerBase : public ACharacter, public IZP_Staggerable, public IZP_Revivable
{
	GENERATED_BODY()

public:
	AZP_ScytheerBase();

	// IZP_Staggerable — enter the Hit flinch and hold (AI paused) for Duration. Used by the
	// player's melee hit and successful block.
	virtual void ReceiveStagger_Implementation(float Duration) override;

	// IZP_Revivable — death-state persistence + objective-driven revival (UZP_DeathSaveComponent).
	virtual void ApplyDeadStateInstant_Implementation() override;
	virtual void ReviveEnemy_Implementation() override;

	// ── Detection ──────────────────────────────────────────────────
	/** Straight-line distance to consider the player for aggro. Final gate is the navmesh
	 *  reachability check — only the same connected navmesh region (= same geometry) qualifies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_DetectionRange = 1000.f;

	/** Seconds between detection probes (LOS trace + navmesh reachability). Ran EVERY FRAME
	 *  before 2026-08-04. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_DetectEvalInterval = 0.15f;

	/** Freeze anim evaluation while off-screen and un-aggroed (VSM page-invalidation relief). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Perf")
	bool bAZP_AnimOnlyWhenRendered = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_LoseSightTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_GiveUpRange = 2200.f;

	/** Max navmesh PATH length (UU) that counts as "same geometry". A closed door blocks the
	 *  navmesh, so even a 200-UU straight-line gap reads as unreachable -> no aggro. An open door
	 *  with navmesh through it reads as reachable -> aggro. 1.6x AZP_DetectionRange covers a typical
	 *  in-room S-shape but rejects "all the way around the building" paths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_MaxReachablePathLength = 1600.f;

	/** Height above the actor pivot the LOS trace starts from (the Scytheer's 'eye'); also used by the aggro debug trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_LOSEyeZOffset = 30.f;

	/** Height above the target's pivot the LOS trace aims at (roughly chest height on the player); also used by the aggro debug trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float AZP_LOSTargetZOffset = 40.f;

	// ── Damage / Death ─────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float AZP_MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float AZP_BodyShotDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float AZP_HeadShotDamage = 50.f;

	/** Hits above this Z (above the actor pivot) count as headshots. Capsule hits carry no bone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float AZP_HeadshotMinZ = 40.f;

	// ── Attack ─────────────────────────────────────────────────────
	/** Melee reach for the base SWIPE. At or within this distance (and off AZP_SwipeCooldown) the
	 *  Scytheer swings a swipe. Beyond this — up to AZP_LungeRange — it LUNGES instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_AttackRange = 200.f;

	/** LEGACY — superseded by AZP_SwipeCooldown / AZP_LungeCooldown (per-attack-type cooldowns). Kept
	 *  so existing Blueprint CDO overrides don't silently drop; no longer read by the attack logic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_AttackCooldown = 0.7f;

	/** Seconds between SWIPES (the base close-range strike). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_SwipeCooldown = 1.0f;

	/** Seconds between LUNGES (the committed gap-closer). Keep this a touch longer than the swipe so
	 *  the lunge reads as a deliberate pounce, not a spam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_LungeCooldown = 1.8f;

	/** Max distance at which the Scytheer will LUNGE. Between AZP_AttackRange and this, a lunge fires
	 *  (off cooldown + line of sight). This band is what stops the player backing up / standing just
	 *  out of melee forever — get within it and the Scytheer pounces. Must be > AZP_AttackRange. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_LungeRange = 500.f;

	/** WIND-UP: seconds the Scytheer coils in place (planted, facing the player, pounce anim frozen on
	 *  its first frame) BEFORE it launches forward. This is the telegraph "hold before lunging" — raise
	 *  it for a longer rear-back, 0 for an instant pounce. NOT the same as AZP_LungeDriveTime (that's how
	 *  long the forward dash lasts once it commits). The lunge sound (AZP_LungeSound) fires at the start
	 *  of this hold, so the wind-up also audibly warns the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_LungeWindupTime = 0.4f;

	/** Forward dash speed (UU/s) during the lunge's drive window — well above AZP_ChaseSpeed so the
	 *  lunge visibly closes distance. Fed via AddMovementInput toward the (tracked) player: unlike a
	 *  ballistic LaunchCharacter it won't tunnel through walls, and the constructor sets
	 *  bCanWalkOffLedges=false so this manual drive also can't step the body off a ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_LungeSpeed = 900.f;

	/** DASH DURATION: max seconds the body drives forward at AZP_LungeSpeed (homing on the player) AFTER
	 *  the wind-up. It's an upper bound — the dash also stops early the moment it reaches melee, so once
	 *  it has closed on you, making this bigger changes nothing (that was the "2 vs 6 looked identical"
	 *  confusion — use AZP_LungeWindupTime for the pre-lunge hold). After the dash it plants and swings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_LungeDriveTime = 0.5f;

	/** Which of the three attack anim slices plays for a SWIPE / a LUNGE (1/2/3 -> AttackN frame range).
	 *  Best-guess default: swipe = slice 1, lunge = slice 2 (the longest, most committed clip). If they
	 *  read backwards in-game just swap these two — no rebuild needed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	int32 AZP_SwipeAttackVariant = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	int32 AZP_LungeAttackVariant = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_AttackDamage = 20.f;

	/** Grace multiplier on AZP_AttackRange at the swing's damage midpoint — the player must be within AZP_AttackRange * this (and visible) for the swipe to connect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_AttackHitRangeMultiplier = 1.4f;

	/** Min seconds between hit-react flinches. Without a CD the Scytheer can be perma-stunlocked
	 *  by sustained fire — every bullet triggers a fresh Hit state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AZP_HitReactCooldown = 1.0f;

	/** Z above the patrol spline's ground point that counts as "on the wall" — controls whether
	 *  aggro transitions through WallDescent or straight into Alert. 100 UU keeps the climb-and-
	 *  back-down on the floor portion of the spline from falsely triggering descent every time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float AZP_OnWallZThreshold = 100.f;

	// ── Movement (drive CharacterMovement) ─────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_WanderSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_ChaseSpeed = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_CombatTurnRate = 300.f;

	/** Movement (UU) below which the chase body counts as 'not moving' for stuck detection (compared squared). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_ChaseStuckMoveThreshold = 30.f;

	/** Seconds stuck during Chase before retrying MoveToActor with a wider acceptance radius (the 2.5f re-hold after the retry is mechanism, not a knob). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_ChaseStuckRepathTime = 2.f;

	/** Seconds of not moving during Chase before the Scytheer de-aggros and returns to patrol/wander (anti door-jam watchdog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_ChaseStuckGiveUpTime = 4.f;

	/** Subtracted from AZP_AttackRange to form the MoveToActor acceptance radius so the body closes inside swing distance; used by both the relaxed stuck-retry (floor 200) and the normal chase move (floor 40). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_ChaseAcceptanceOffset = 60.f;

	/** Acceptance/arrival radius for non-chase MoveToLocation calls — ReturnToPatrol arrival + goal and wander-point moves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float AZP_MoveAcceptanceRadius = 80.f;

	// ── Patrol (optional designer-guided path) ─────────────────────
	/** If set, Wander walks back and forth along this spline instead of picking random navmesh points.
	 *  Drop a ZP_ScytheerClimbPath actor in the level, shape its spline, then assign here. The path's
	 *  points must sit on the navmesh for the AI to walk between them. Chase still uses navmesh; on
	 *  losing the player the Scytheer rejoins the nearest point on the path and resumes patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	TObjectPtr<AZP_ScytheerClimbPath> AZP_PatrolPath;

	/** Distance between consecutive patrol waypoints along the spline (UU). Smaller = the AI re-paths
	 *  more often (smoother curves but more CPU); larger = it cuts corners on tight bends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float AZP_PatrolStep = 200.f;

	/** Max rotational speed (deg/s) the body uses while patrolling. Without a cap, reversing
	 *  direction at the spline endpoints flips the body 180° in a single frame (snap). 360 = the
	 *  reversal takes 0.5s; lower = more deliberate-looking turns. Set very high (e.g. 9999) for
	 *  the original instant-flip behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float AZP_PatrolTurnRate = 360.f;

	// ── Wander (random-roam fallback when AZP_PatrolPath is unset) ─────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float AZP_WanderRadius = 600.f;

	/** 2D distance to the random wander destination that counts as arrival, triggering the pause-then-repick cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float AZP_WanderArriveRadius = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float AZP_PauseMin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float AZP_PauseMax = 3.5f;

	// ── Alert (plays after first detection, before Chase) ──────────
	/** Seconds the Scytheer holds the idle pose on first detection before breaking into the chase.
	 *  DEFAULT 0 = no pause: it fires the alert SFX and runs the same frame (the Shambler wants a
	 *  hold here; the Scytheer does not). Raise it to reinstate a stand-and-notice beat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Alert")
	float AZP_AlertHoldTime = 0.0f;

	// ── Audio placeholders ─────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_AlertSound;

	/** LEGACY/fallback attack sound. Now only used if the per-attack AZP_SwipeSound / AZP_LungeSound
	 *  slot is empty — swipe and lunge each have their own sound below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_AttackSound;

	/** Sound played when the Scytheer starts a SWIPE (the base close-range strike). Falls back to
	 *  AZP_AttackSound if left empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_SwipeSound;

	/** Sound played when the Scytheer starts a LUNGE — fires at the START of the wind-up, so it doubles
	 *  as the pounce telegraph. Falls back to AZP_AttackSound if left empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_LungeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_LurkSound;

	// ── Footsteps (distance-based; see PlayFootstep) ───────────────
	/** Footstep one-shot, played once per AZP_FootstepStride of 2D travel. Defaults to
	 *  /Game/Audio/Scytheer/SFX_Scytheer_Footsteps in LoadSFXDefaults. Distance is measured from the
	 *  body's OWN position delta (not GetVelocity()) so it cadence-matches every gait: the navmesh
	 *  wander/chase AND the teleport-driven spline patrol, where GetVelocity() reads 0 and the
	 *  Shambler's velocity scheme would be silent. 0 volume = silent feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AZP_FootstepSound;

	/** Footstep VOLUME. 0 = silent feet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	float AZP_FootstepVolume = 1.f;

	/** Distance (UU) of 2D travel per footstep. Distance-based so cadence scales with actual speed —
	 *  slow wander steps slowly, chase steps fast — with zero per-clip work. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	float AZP_FootstepStride = 80.f;

	/** Random pitch spread per step (1 +/- this) so a single footstep asset never reads as a
	 *  mechanical loop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	float AZP_FootstepPitchVar = 0.08f;

	/** LEGACY — no longer used. Voice SFX route through UZP_SFXStatics (Far carry, ~120 m natural
	 *  falloff, C++-owned) so carry can't silently drift in a .uasset; the old SA_EnemyVoice asset
	 *  fell silent at ~15 m (inner 350 + falloff 1200, linear). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundAttenuation> AudioAttenuation;

	// ── Animation source ───────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	TObjectPtr<UAnimSequence> AZP_SingleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_WalkStartFrame = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_WalkEndFrame = 33;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_RunStartFrame = 35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_RunEndFrame = 53;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_IdleStartFrame = 55;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_IdleEndFrame = 190;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack1StartFrame = 192;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack1EndFrame = 222;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack2StartFrame = 223;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack2EndFrame = 288;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack3StartFrame = 291;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_Attack3EndFrame = 321;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_HitStartFrame = 323;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_HitEndFrame = 353;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_DieStartFrame = 354;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 AZP_DieEndFrame = 500;

	UPROPERTY(BlueprintReadOnly, Category = "Scytheer|State")
	EScytheerState State = EScytheerState::Wander;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bDead = false;
	bool bAggro = false;
	// Detection probe cache (refreshed every AZP_DetectEvalInterval; see Tick)
	float DetectEvalAccum = 1000.f;
	bool bCachedSee = false;
	bool bCachedReachable = false;
	bool bRestoringDeadState = false; // true only while ApplyDeadStateInstant runs -> Die entry stays silent (no death cry replay on load)
	float LostSightTimer = 0.f;
	float PauseTimer = 0.f;
	bool bWanderMoving = false;
	FVector WanderDest = FVector::ZeroVector;
	float PatrolDistance = 0.f;
	int32 PatrolDir = 1;
	/** Cached so we can restore CharacterMovement.MovementMode when leaving spline-direct patrol. */
	uint8 PreSplineMovementMode = 1; // MOVE_Walking
	// Per-attack-type cooldown clocks (seconds; init far in the past so both are ready at spawn).
	double LastSwipeTime = -1000.0;
	double LastLungeTime = -1000.0;
	double AttackStartTime = 0.0;
	bool bAttackHitFired = false;
	/** Which attack the current/next Attack state plays. Drives the anim slice AND whether the body
	 *  drives forward (lunge) or plants (swipe). */
	EScytheerAttackType PendingAttackType = EScytheerAttackType::Swipe;
	/** CharacterMovement's authored MaxAcceleration, cached at BeginPlay so the lunge can spike it for
	 *  a snappy dash and restore it afterwards (default accel ramps too slowly to read as a lunge). */
	float DefaultMaxAccel = 2048.f;
	double LastHitReactTime = -1000.0;
	/** While true, the Hit state holds (stays staggered) until StaggerHandle fires instead of
	 *  auto-returning to Chase/Wander when the flinch clip ends. Set by ReceiveStagger. */
	bool bStaggerHold = false;
	FTimerHandle StaggerHandle;
	float ChaseStuckTimer = 0.f;
	FVector LastChaseStuckLoc = FVector::ZeroVector;

	// distance-based footsteps: accumulate 2D position delta, one step per AZP_FootstepStride.
	float StepDistanceAccum = 0.f;
	FVector LastFootstepLoc = FVector::ZeroVector;
	int32 PendingAttackVariant = 1;
	double StateEnteredAt = 0.0;

	// anim slice
	float SegStartT = 0.f;
	float SegEndT = 0.f;
	bool bSegLoops = true;
	float SecondsPerFrame = 1.f / 30.f;
	float ClipLen = 0.f;

	UPROPERTY() TObjectPtr<UZP_HealthComponent> Health;
	UPROPERTY() TObjectPtr<AAIController> AICon;

	void EnterState(EScytheerState NewState);
	void StartSegment(int32 FrameStart, int32 FrameEnd, bool bLoop);
	void OnSegmentComplete();
	void TickAnim();
	/** Fill any SFX slot the Blueprint hasn't overridden with the Crawler/Shambler placeholders.
	 *  Called from BeginPlay, NOT the constructor — ConstructorHelpers asset loading during CDO
	 *  construction crashes when the target asset is renamed/moved (Shambler hit that in this same
	 *  session). LoadObject in BeginPlay is the safe pattern. */
	void LoadSFXDefaults();
	/** Play one footstep one-shot at the body (Room carry, AZP_FootstepVolume / AZP_FootstepPitchVar). */
	void PlayFootstep();
	float Frame2Time(int32 Frame) const;
	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;
	/** Is the player reachable on the same connected navmesh region within AZP_MaxReachablePathLength?
	 *  Closed doors block navmesh, so this returns false through any solid barrier. The
	 *  "same geometry" gate the dev asked for. */
	bool IsPlayerReachable(const AActor* Target) const;
	void PickNewWanderPoint();
	void SetMaxWalkSpeed(float Speed);
	void FaceTargetSmooth(float DeltaTime);

	UFUNCTION()
	void OnOwnerDied();

	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir, const UDamageType* DamageType, AActor* DamageCauser);

	/** Compose a rotator whose +X axis points along Forward and whose +Z points along Up. Used to
	 *  align the Scytheer's body to the spline tangent + the per-point wall normal so it can stand
	 *  on walls / ceilings during patrol. Up is orthogonalized against Forward first. */
	static FRotator MakeOrientation(const FVector& Forward, const FVector& Up);
};
