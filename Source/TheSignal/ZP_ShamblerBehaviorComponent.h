// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_ShamblerBehaviorComponent
 *
 * Purpose: Ground-zombie ("Shambler") AI. Wanders a small radius until it gets a clear line of sight
 *          to the player within DetectionRange, screams (alert), then chases. Attacks (alternating
 *          left/right swipe) when in range. Gives up and returns to wandering if it loses line of
 *          sight / the player leaves its space for LoseSightTime.
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
#include "ZP_ShamblerBehaviorComponent.generated.h"

class UAnimSequence;
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
	Attack    // mid-swipe
};

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_ShamblerBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_ShamblerBehaviorComponent();

	// --- Detection ---
	/** Max distance (UU) at which it can notice the player. Real walls/doors are meant to gate aggro via
	 *  line of sight, NOT this range. Left at the original 1800; let LOS do the containing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float DetectionRange = 1800.f;

	/** Seconds without a clear sightline to the player before it gives up and wanders off.
	 *  This is the "wait at the shut door" beat — it pursues to the last spot, then resets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float LoseSightTime = 5.f;

	/** Hard leash (UU): beyond this it gives up immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float GiveUpRange = 3000.f;

	/** dot(forward, dirToPlayer) it must exceed to notice you — it has to be roughly facing you.
	 *  0.25 ~ a 75-degree half-cone in front. Lower = wider awareness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Detect")
	float FacingThreshold = 0.25f;

	// --- Damage / death (player shooting the Shambler) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float MaxHealth = 100.f;

	/** Damage per body shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float BodyShotDamage = 20.f;

	/** Damage per headshot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float HeadShotDamage = 50.f;

	/** A hit this many UU above the body centre counts as a headshot (capsule hits carry no bone, so
	 *  headshots are judged by height). Capsule half-height is ~88; ~55 ≈ upper chest/head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Damage")
	float HeadshotMinZ = 55.f;

	// --- Attack ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AttackRange = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AttackCooldown = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AttackDamage = 20.f;

	/** Seconds into the swipe the hit lands (damage + your vignette). The clip is a wind-up then a
	 *  strike near the END, so this is late by default — nudge it to the exact contact frame. Must be < AttackDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AttackHitTime = 1.3f;

	/** How long one swing lasts before the next chains (or it recovers to chase). Long enough to let the
	 *  full swing play; shorten for a faster (but clipped) flurry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float AttackDuration = 1.7f;

	/** Seconds it holds the scream (stationary, plays the alert) before breaking into the chase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Attack")
	float ScreamHoldTime = 2.0f;

	// --- Speeds (set on the owner's CharacterMovement) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float WanderSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float ChaseSpeed = 250.f;

	/** Degrees/sec it pivots to FACE the player during combat (scream / chase / attack). Fast enough to
	 *  track you circling it, but not instant. Wander still turns via movement orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Move")
	float CombatTurnRate = 300.f;

	// --- Wander ("shambly" — short bursts, frequent stops, mid-leg stumbles) ---
	/** How far each wander LEG walks (UU). Smaller radius keeps the body in a tighter footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WanderRadius = 180.f;

	/** Min straight-line distance (UU) a candidate wander point must be from the body's current
	 *  position. Without this the picker can land within 80 UU and the leg ends instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WanderMinLegDistance = 100.f;

	/** Target dot(forward, dirToCandidate) for picked legs. 1.0 = straight ahead (linear path),
	 *  0.0 = perpendicular (90° turn), -1.0 = behind. ~0.5 (~60° off forward) gives curving,
	 *  organic-looking wander paths instead of straight lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WanderTargetDot = 0.5f;

	/** Walk-phase duration (s). The Shambler walks for this many seconds (picking new legs as it
	 *  arrives at each dest), then drops to idle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WalkDurationMin = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WalkDurationMax = 6.0f;

	/** Idle-phase duration (s). The Shambler stops, plays the zombie idle animation, holds for this
	 *  many seconds, then resumes walking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float IdleDurationMin = 6.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float IdleDurationMax = 9.0f;

	/** Minimum seconds a wander leg must last before its "did I arrive?" check can fire. Without
	 *  this, when the AI rejects the picked path (unreachable) the body instantly "arrives"
	 *  (DistToDest never changes from start), the leg ends, a new pause begins — the rapid
	 *  pause→leg→pause cycle the dev called "firing off a bunch of decisions in a short period". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float MinLegDurationSec = 1.0f;

	/** Z offset (UU) applied to the mesh while the idle pose is showing. Compensates for the idle
	 *  anim's pelvis sitting higher than the walk's — without it, the feet visibly lift off the
	 *  ground at WALK→IDLE. Negative pulls the body down; tune until the foot plant matches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float IdleMeshZOffset = -8.f;

	/** Blend-in time (s) for the idle slot anim when entering the IDLE phase. Long enough to cover
	 *  the CMC's natural deceleration from walking speed → 0. Shorter = jagged snap; longer =
	 *  walk pose leaks through the slot. 0.3 covers a default braking ramp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float IdleBlendInTime = 0.3f;

	/** Blend-out time (s) for the idle slot anim when leaving the IDLE phase. Long enough that the
	 *  BlendSpace below has time to ramp up from speed=0 to walking before the idle pose has fully
	 *  faded. 0.3 reads as a natural drift back into a walk cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float IdleBlendOutTime = 0.3f;

	/** Seconds after entering IDLE before the CMC is hard-locked to MOVE_None. During this delay
	 *  the CMC brakes from walk speed to ~0 naturally (smooth deceleration). After the delay,
	 *  MOVE_None freezes any residual nudges (RVO, leftover acceleration, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float IdleLockDelay = 0.35f;

	/** Per-second probability of interrupting an in-progress wander leg with a mid-leg stumble
	 *  (briefly stops moving, then resumes toward the same target). The dropping-then-resuming
	 *  motion is what reads as "zombie hesitation". 0 = no stumbles, 1 = stumble basically every leg.
	 *  Kept low so the wander stays smooth — high values stack pauses on top of leg-end pauses
	 *  and the body looks like it's stop-pivot-stop-pivot constantly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float StumbleChancePerSec = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float StumbleMin = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float StumbleMax = 0.8f;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float LurkRange = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float LurkIntervalMin = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Audio")
	float LurkIntervalMax = 13.f;

	// --- Animations (defaulted to the retargeted Shambler clips) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AttackLAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> AttackRAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> ScreamAnim;

	/** Directional death clips (retargeted FPP_Dag_Death1 = front fall, Death2 = back fall). Played
	 *  on death and held on the final frame as the corpse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> DeathFrontAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> DeathBackAnim;

	/** Directional hit-reaction clips (retargeted HitZombie_Front/Back). Played on non-lethal damage
	 *  — front hit when shot from the front, back hit when shot from behind. Mirrors the death
	 *  direction logic: same dot-product test against the actor's forward at the moment of impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> HitFrontAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	TObjectPtr<UAnimSequence> HitBackAnim;

	/** Speed (UU/s) at which the walk clip's stride looks natural. The clip's play rate scales with
	 *  actual speed / this, so the feet keep pace instead of sliding. Tune until skate is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AnimWalkRefSpeed = 150.f;

	/** The scream clip has no root motion but its retargeted pose floats off the ground. This nudges the
	 *  mesh's Z only during the scream to ground it. -16 matches the same pelvis-lift compensation
	 *  the idle clip uses, doubled for the scream's more-upright pose. 0 = no change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float ScreamMeshZOffset = -16.f;

	/** The death clips have no root motion, so the corpse lands floating at hip height. This lowers the
	 *  mesh by this much (UU) over the death clip so it settles on the ground. Tune until it grounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float DeathDropZ = 90.f;

	/** Finish the drop this many seconds BEFORE the clip ends, so it lands with the fall instead of
	 *  crawling the whole clip. The drop is also eased (fast then settle) to track the visual fall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float DeathDropLead = 0.5f;

	/** Eval period (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler")
	float EvalInterval = 0.25f;

	/** Min seconds between hit-react plays so consecutive shots don't constantly retrigger the
	 *  flinch (which would lock the Shambler in a perpetual loop and let the player kite for free). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float HitReactCooldown = 1.0f;

	/** Yaw deg/sec the body uses to pre-rotate toward the next wander dest during a pause. Picking
	 *  the next dest BEFORE the pause and turning during it means the next leg starts already
	 *  facing the right way — no awkward stand-still pivot the moment locomotion resumes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float PauseTurnRate = 90.f;

	UPROPERTY(BlueprintReadOnly, Category = "Shambler|State")
	EShamblerState State = EShamblerState::Wander;

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
	/** Play a stationary full-body one-shot (scream/swipe) by briefly swapping the mesh to single-clip
	 *  mode; locomotion (the AnimBP) is restored on the next Wander/Chase state. No AnimGraph slot needed. */
	void PlayOneShot(UAnimSequence* Anim);
	/** Play a clip on the AnimBP's DefaultSlot looped indefinitely (until StopSlotLoop). Used to
	 *  hold a real Idle pose during wander pauses — without it the BS_Shambler BlendSpace shows
	 *  walk-at-speed-0 while stopped, which reads as "marching in place" not "idle". */
	void PlaySlotLoop(UAnimSequence* Anim);
	/** Stop whatever is currently on the DefaultSlot so locomotion takes back over. */
	void StopSlotLoop();
	/** Re-activate the locomotion AnimBP if a one-shot left the mesh in single-clip mode. */
	void EnsureLocomotion();

	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;             // clear sightline, walls block (same geometry)
	/** Pick a random navmesh point in WanderRadius and write it to WanderDest. Returns true if
	 *  found. Does NOT issue MoveToLocation — separated so we can pre-pick at pause-entry and
	 *  let the body rotate toward the new dest while standing idle. */
	bool PickNewWanderPoint();
	/** Issue MoveToLocation toward WanderDest. Call after pre-picking with PickNewWanderPoint. */
	void StartWanderLeg();
	/** Smoothly rotate the owner's yaw toward the current target at CombatTurnRate (per-frame). */
	void FaceTargetSmooth(float DeltaTime);
	void BeginAttack();
	void ApplyAttackDamage();

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
	 *  EvalInterval (0.25s) for the next Evaluate to fire — that gap is what reads as "still pausing". */
	UFUNCTION()
	void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	/** Delayed-by-IdleLockDelay callback: once the CMC has braked from walk speed to ~0, lock the
	 *  movement mode to MOVE_None and zero velocity. Lets the deceleration play out visibly under
	 *  the slot blend instead of snapping. */
	UFUNCTION()
	void LockIdleMovement();

	/** PROBE: trace a Visibility ray straight through the mesh center to prove whether its collision is hittable. */
	void ProbeShootable();

	void UpdateLurk(float DistToPlayer);

	UPROPERTY() TObjectPtr<ACharacter> Owner;
	UPROPERTY() TObjectPtr<AAIController> AICon;
	UPROPERTY() TObjectPtr<UZP_EnemyAudioComponent> Audio;
	UPROPERTY() TObjectPtr<UZP_HealthComponent> Health;
	UPROPERTY() TObjectPtr<AActor> Target;
	float MeshBaseRelZ = 0.f;
	FVector SpawnLocation = FVector::ZeroVector; // cached at BeginPlay; wander stays tethered around this

	FTimerHandle EvalTimer;
	FTimerHandle AttackHitTimer;
	FTimerHandle ProbeTimer;
	FTimerHandle IdleLockTimer; // delays MOVE_None until braking deceleration finishes (smooth WALK→IDLE)

	float LostSightTimer = 0.f;
	bool bWanderMoving = false;
	bool bStumbling = false;
	double StumbleEndTime = 0.0;
	double LastHitReactTime = -1000.0;
	float IdleDuration = 1.f;  // randomized 6-9s at idle-entry
	float WalkDuration = 4.f;  // randomized 3-6s at walk-entry
	FVector WanderDest = FVector::ZeroVector;
	double LastAttackTime = -1000.0;
	bool bAttackIsLeft = false;
	bool bLastHitFront = true; // which side the last damage came from -> death-fall direction
	double DeathStartTime = 0.0;
	float DeathAnimLen = 0.f;
	bool bDropping = false;
	float StateTimer = 0.f;
	bool bDead = false;

	// lurk audio
	float LurkTimer = 0.f;
	float LurkInterval = 8.f;
	bool bLurkInit = false;
	bool bWasInLurkRange = false;
};
