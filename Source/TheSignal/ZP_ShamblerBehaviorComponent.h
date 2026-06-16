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

	// --- Wander ---
	/** How far each wander LEG walks (UU) before stopping to pause. It walks one smooth leg, pauses,
	 *  then slow-turns and walks another — no jerky re-pathing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float WanderRadius = 600.f;

	/** Short pause between legs (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float PauseMin = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float PauseMax = 2.0f;

	/** Chance (0-1) a pause is instead a long idle "beat", and its length range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float LongPauseChance = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float LongPauseMin = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Wander")
	float LongPauseMax = 5.5f;

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

	/** Speed (UU/s) at which the walk clip's stride looks natural. The clip's play rate scales with
	 *  actual speed / this, so the feet keep pace instead of sliding. Tune until skate is gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float AnimWalkRefSpeed = 150.f;

	/** The scream clip has no root motion but its retargeted pose floats off the ground. This nudges the
	 *  mesh's Z only during the scream to ground it (try ~ -15). 0 = no change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shambler|Anim")
	float ScreamMeshZOffset = 0.f;

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

	UPROPERTY(BlueprintReadOnly, Category = "Shambler|State")
	EShamblerState State = EShamblerState::Wander;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void Evaluate();
	void SetState(EShamblerState NewState);
	void SetSpeed(float Speed);
	/** Play a stationary full-body one-shot (scream/swipe) by briefly swapping the mesh to single-clip
	 *  mode; locomotion (the AnimBP) is restored on the next Wander/Chase state. No AnimGraph slot needed. */
	void PlayOneShot(UAnimSequence* Anim);
	/** Re-activate the locomotion AnimBP if a one-shot left the mesh in single-clip mode. */
	void EnsureLocomotion();

	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;             // clear sightline, walls block (same geometry)
	void PickNewWanderPoint();
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

	/** PROBE: trace a Visibility ray straight through the mesh center to prove whether its collision is hittable. */
	void ProbeShootable();

	void UpdateLurk(float DistToPlayer);

	UPROPERTY() TObjectPtr<ACharacter> Owner;
	UPROPERTY() TObjectPtr<AAIController> AICon;
	UPROPERTY() TObjectPtr<UZP_EnemyAudioComponent> Audio;
	UPROPERTY() TObjectPtr<UZP_HealthComponent> Health;
	UPROPERTY() TObjectPtr<AActor> Target;
	float MeshBaseRelZ = 0.f;

	FTimerHandle EvalTimer;
	FTimerHandle AttackHitTimer;
	FTimerHandle ProbeTimer;

	float LostSightTimer = 0.f;
	bool bWanderMoving = false;
	float PauseTime = 1.f;
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
