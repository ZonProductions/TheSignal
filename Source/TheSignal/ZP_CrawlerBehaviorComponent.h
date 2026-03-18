// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_CrawlerBehaviorComponent
 *
 * Purpose: Wall-dwelling crawler behavior. Three states:
 *
 *          Patrol  — deliberate wander, walls as territory, 250 UU/s
 *          Hunt    — direct pursuit (300 UU/s), gaze freeze at 50m+ on walls
 *          Attack  — slam (0.3s rear-up, 25 dmg, 1.5s cooldown)
 *
 *          No Alert, no Stalk. Sees you -> comes for you. Period.
 *          Indoor/outdoor detection via roof trace adapts hearing/detection distances.
 *
 * Owner Subsystem: EnemyAI
 *
 * Blueprint Extension Points:
 *   - PatrolPoints array: set per-instance in editor
 *   - OnStateChanged delegate: BP hook for animations, SFX, VFX per state
 *   - StartHunt / ReturnToPatrol: callable from BP
 *
 * Dependencies:
 *   - UPawnSensingComponent (auto-bound if present on owner)
 *   - UZP_CrawlerMovementComponent (MoveTarget/MoveTargetActor + launch API)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_CrawlerBehaviorComponent.generated.h"

class UPawnSensingComponent;
class UZP_CrawlerMovementComponent;

UENUM(BlueprintType)
enum class ECrawlerState : uint8
{
	Patrol,  // Find ceiling, perch, wait for prey
	Attack   // Drop from ceiling, slam, return to perch
};

UENUM(BlueprintType)
enum class EAttackType : uint8
{
	None,
	Slam
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrawlerStateChanged, ECrawlerState, OldState, ECrawlerState, NewState);

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_CrawlerBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_CrawlerBehaviorComponent();

	// --- Configuration (set per-instance in editor) ---

	/** World-space patrol waypoints. Creature cycles through these in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolPoints;

	/** Distance threshold to consider "arrived" at a waypoint (UU). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float ArrivalThreshold = 300.0f;

	/** How far the creature wanders from its spawn point when idle (no patrol points). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float WanderRadius = 1500.0f;

	/** Min/max pause duration at each waypoint (seconds). Random range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float WaypointPauseMin = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float WaypointPauseMax = 1.5f;

	/** Speed during patrol state (UU/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Speed")
	float PatrolSpeed = 250.0f;

	/** Speed during attack chase (UU/s). Slightly slower than player forward sprint (115)
	 *  so player can outrun forward, but catches backpedaling players (115 * 0.55 = 63). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Speed")
	float HuntSpeed = 105.0f;

	/** Max distance (UU) for direct player detection. Runs every eval tick — doesn't depend on PawnSensing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Detection")
	float DetectionRange = 600.0f;

	/** Maximum height (UU) above home location during patrol.
	 *  Prevents climbing to the moon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float MaxPatrolHeight = 800.0f;

	/** Dot product threshold for gaze detection. 0.85 ~ 30deg cone from camera forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Gaze")
	float GazeThreshold = 0.85f;

	/** Minimum distance (UU) for gaze freeze to activate. Below this, creature keeps moving.
	 *  5000 UU = 50m — freeze is an "is that something?" moment at the edge of perception, not close range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Gaze")
	float GazeFreezeMinDistance = 5000.0f;

	/** How long without seeing player during hunt before returning to patrol (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Hunt")
	float HuntLoseSightTime = 10.0f;

	/** Max distance (UU) to maintain hunt. Beyond this, creature gives up and returns to patrol.
	 *  600 UU = 6m — about one office room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Hunt")
	float HuntMaxRange = 600.0f;

	/** Range (UU) within which the creature "hears" the player during Hunt, resetting sight timer.
	 *  Prevents the shoot-and-run exploit where player breaks LOS and creature gives up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Hunt")
	float HearingRange = 600.0f;

	/** Seconds between path recalculations during Hunt. Prevents indecisive fence-hopping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Hunt")
	float RetargetInterval = 3.0f;

	/** Creature must close this fraction of distance or retarget triggers. 0.9 = must close 10%. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Hunt")
	float RetargetProgressThreshold = 0.9f;

	/** Distance (UU) the creature must move within StallTimeout or target is re-evaluated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Stall")
	float StallDistance = 50.0f;

	/** Time (seconds) without meaningful movement before stall recovery triggers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Stall")
	float StallTimeout = 1.5f;

	/** How often (seconds) the behavior evaluation timer fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float EvalInterval = 0.4f;

	/** If >= 0, overrides the creature's Monster Randomizer seed for deterministic appearance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	int32 CrawlerSeed = -1;

	/** If true, behavior starts automatically from BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	bool bAutoInitialize = true;

	/** Seconds to wait before starting patrol. Lets creatures reach wall positions before player arrives.
	 *  Set per-instance: stagger values (0, 3, 8, etc.) so creatures aren't all in sync. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Patrol")
	float InitialPatrolDelay = 2.0f;

	// --- Wall Seeking ---

	/** How far (UU) to trace horizontally when searching for walls to climb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|WallSeek")
	float WallSeekRadius = 2000.0f;

	/** Number of radial directions to trace when searching for walls. More = better coverage, slower. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|WallSeek")
	int32 WallSeekTraceCount = 12;

	// --- Threat System ---

	/** Threat gained per second while player is in LOS (scales with proximity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Threat")
	float ThreatPerSecondLOS = 20.0f;

	/** Threat decays at this rate per second when player is NOT visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Threat")
	float ThreatDecayRate = 5.0f;

	/** Threat level that forces Hunt regardless of current state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Threat")
	float ThreatThreshold = 100.0f;

	/** Threat added instantly when a gunshot is heard. One shot nearly maxes it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Threat")
	float ThreatFromGunshot = 100.0f;

	/** Radius (UU) that gunfire alerts creatures. Office-scale: same floor section. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Threat")
	float GunshotAlertRadius = 2500.0f;

	/** Current threat level (read-only from BP). */
	UPROPERTY(BlueprintReadOnly, Category = "Behavior|Threat")
	float ThreatLevel = 0.0f;

	// --- Attack System ---

	/** Max horizontal distance (UU) for creature to initiate a ground attack during Hunt.
	 *  200 = personal space violation — creature must be close. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Attack")
	float AttackRange = 200.0f;

	/** Cooldown (seconds) between ground attacks. Relentless — 1.5s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Attack")
	float AttackCooldown = 1.5f;

	/** Damage dealt by lunge and drop attacks on contact near player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Attack")
	float LungeDamage = 25.0f;

	/** Proximity radius (UU) from creature to player for slam to deal damage on impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Attack")
	float SlamDamageRadius = 250.0f;

	/** How long the drop attack rear-up takes before impact (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Attack")
	float DropAttackHoldTime = 0.3f;

	// --- Indoor/Outdoor Detection ---

	/** Cached indoor status. True if roof trace hit a ceiling above the creature. */
	UPROPERTY(BlueprintReadOnly, Category = "Behavior|Environment")
	bool bIsIndoors = false;

	/** Upward trace distance (UU) to detect ceilings for indoor/outdoor classification. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior|Environment")
	float IndoorTraceDistance = 3000.0f;

	// --- State (read-only from BP) ---

	UPROPERTY(BlueprintReadOnly, Category = "Behavior|State")
	ECrawlerState CurrentState = ECrawlerState::Patrol;

	UPROPERTY(BlueprintReadOnly, Category = "Behavior|State")
	int32 CurrentPatrolIndex = 0;

	// --- Delegates ---

	/** Fired on every state transition. Use for animation, SFX, VFX triggers. */
	UPROPERTY(BlueprintAssignable, Category = "Behavior|Events")
	FOnCrawlerStateChanged OnStateChanged;

	// --- Public API ---

	/** Initializes behavior: starts eval timer, sets patrol state. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void InitializeBehavior();

	/** Force hunt a specific target (usually the player). Goes directly to Hunt — no Alert delay. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void StartHunt(AActor* Target);

	/** Reset to patrol behavior from current position. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ReturnToPatrol();

	/** Called when a gunshot is heard. Instantly spikes threat and reveals shooter location. */
	void OnHearGunshot(const FVector& NoiseLocation, AActor* Shooter);

	/** Alert all creatures within radius. Call from weapon fire. */
	static void BroadcastGunshot(UWorld* World, const FVector& Location, float Radius, AActor* Shooter);

private:
	/** Static registry of all active behavior components.
	 *  Bypasses FindComponentByClass which fails when BP component name
	 *  doesn't match C++ class name (CoreRedirect rename issue). */
	static TArray<UZP_CrawlerBehaviorComponent*> AllCrawlers;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- Internal state ---
	void SetState(ECrawlerState NewState);
	void EvaluateBehavior();
	void OnSeePawn(APawn* SeenPawn);

	/** Set MaxFlySpeed on the owner's CMC. */
	void SetSpeed(float Speed);

	/** Set CMC->MoveTarget to PatrolPoints[CurrentPatrolIndex]. */
	void AdvanceToNextWaypoint();

	/** Check if the player camera is looking at this creature. Includes LOS check. */
	bool IsPlayerLookingAtCreature() const;

	/** Upward trace to detect ceiling — sets bIsIndoors. */
	void UpdateIndoorStatus();

	/** Distance calculation that adapts to indoor/outdoor. Indoor: 3D, Outdoor: 2D. */
	float GetEffectiveDistance(const FVector& From, const FVector& To) const;

	/** Radial trace to find nearest wall surface. Returns true if wall found. */
	bool FindNearestWall(FVector& OutWallPoint, FVector& OutWallNormal) const;

	/** True if the creature is on the ground (not climbing, not launching, not slamming). */
	bool IsOnGround() const;

	// --- Attack methods ---
	void BeginSlamAttack();
	void ApplyAttackDamage(float Damage, float Radius);

	// --- Cached references ---

	UPROPERTY()
	TObjectPtr<AActor> ChaseTargetActor;

	UPROPERTY()
	TObjectPtr<UZP_CrawlerMovementComponent> CrawlerCMC;

	// --- Timers ---
	FTimerHandle EvalTimerHandle;
	FTimerHandle WaypointPauseTimerHandle;

	// --- Tracking ---
	FVector HomeLocation = FVector::ZeroVector;
	FVector PerchLocation = FVector::ZeroVector; // Ceiling spot above home — creature hides here
	bool bReachedPerch = false; // True once creature arrives at ceiling perch
	FVector LastKnownPlayerLocation = FVector::ZeroVector;
	FVector StallCheckLocation = FVector::ZeroVector;
	float StallTimer = 0.f;
	float StateTimer = 0.f;
	float LastSeenTimer = 0.f;
	bool bIsPaused = false;

	// --- Hunt retarget throttle ---
	double LastRetargetTime = 0.0;
	float DistanceAtLastRetarget = MAX_FLT;

	// --- Gaze tracking (Hunt) ---
	bool bBeingObserved = false;
	float ObservedTimer = 0.f;

	// --- Attack state ---
	EAttackType CurrentAttackType = EAttackType::None;
	float AttackPhaseTimer = 0.f;
	double LastAttackTime = 0.0;
	bool bAttackDamageApplied = false;
	FTimerHandle AttackTimerHandle;

	// --- Wall seeking ---
	bool bSeekingWall = false;
};
