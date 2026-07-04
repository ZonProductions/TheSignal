// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_CrawlerBehaviorComponent
 *
 * Purpose: Brain for the wall-ambush crawler. The crawler is PLACED on a wall (the wall is its
 *          floor) and waits there. It leaves the wall exactly once, then hunts on the ground — it
 *          NEVER re-perches.
 *
 *            PERCHED — dormant until the player shares its space (clear sightline) and is within
 *                      AZP_PerchDormantRange (40m). Then:
 *                        • player within AZP_DropProximity (5m)                  -> SILENT drop, ground hunt
 *                        • direct stare >= AZP_GazeProvokeTime (2s) within AZP_GazeLaunchRange (20m) -> LAUNCH
 *            GROUNDED — pursue the player on the floor; slam when in range.
 *
 *          CurrentState stays { Patrol, Attack } for Blueprint anim/SFX hooks: Patrol = perched OR
 *          hunting (not mid-attack), Attack = a launch/slam is in progress.
 *
 * Owner Subsystem: EnemyAI
 *
 * Dependencies: UZP_CrawlerMovementComponent (cling/launch/slam/ground), UZP_EnemyAudioComponent
 *               (voice), UZP_HealthComponent (death/hit).
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_CrawlerBehaviorComponent.generated.h"

class UZP_CrawlerMovementComponent;
class UZP_EnemyAudioComponent;

UENUM(BlueprintType)
enum class ECrawlerState : uint8
{
	Patrol,  // perched OR hunting on the ground (not mid-attack)
	Attack   // a launch/slam is in progress
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrawlerStateChanged, ECrawlerState, OldState, ECrawlerState, NewState);

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_CrawlerBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_CrawlerBehaviorComponent();

	// --- Perch ---
	/** Adhere to a wall the designer placed this crawler against, on spawn (the wall is its floor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	bool bAZP_SpawnOnWall = true;

	/** Max distance (UU) from the spawn point to find a wall to adhere to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_SpawnWallSnapRange = 300.f;

	/** While perched, the player is ignored entirely beyond this range (UU). 4000 = 40m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_PerchDormantRange = 4000.f;

	/** Player closer than this (UU) while perched -> SILENT drop into a ground hunt. 500 = 5m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_DropProximity = 500.f;

	/** Max range (UU) at which a sustained stare provokes a launch pounce. 2000 = 20m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_GazeLaunchRange = 2000.f;

	/** Seconds the player must stare directly at the perched crawler before it launches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_GazeProvokeTime = 2.0f;

	/** Min seconds between gaze-provoke alert hisses so a held stare doesn't spam it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_GazeAlertCooldown = 3.0f;

	/** Low-pass frequency (Hz) on the gaze-provoke alert instance for a muffled hiss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_GazeAlertLowPassHz = 5000.f;

	/** dot(view, dirToCrawler) threshold for "looking right at it". 0.85 ~ 30deg cone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Perch")
	float AZP_GazeThreshold = 0.85f;

	// --- Hunt / attack ---
	/** Ground pursuit speed (UU/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Hunt")
	float AZP_HuntSpeed = 220.f;

	/** Range (UU) within which a grounded crawler (re)acquires the player on a clear sightline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Hunt")
	float AZP_DetectionRange = 3000.f;

	/** Horizontal distance (UU) at which a grounded crawler slams. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Attack")
	float AZP_AttackRange = 200.f;

	/** Seconds between attacks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Attack")
	float AZP_AttackCooldown = 1.5f;

	/** Damage per landed attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Attack")
	float AZP_AttackDamage = 25.f;

	/** Proximity (UU) to the player at impact for an attack to connect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Attack")
	float AZP_AttackHitRadius = 250.f;

	/** Slam wind-up/strike duration (seconds) before the impact check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Attack")
	float AZP_SlamHoldTime = 0.3f;

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Audio")
	float AZP_LurkRange = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Audio")
	float AZP_LurkIntervalMin = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Audio")
	float AZP_LurkIntervalMax = 15.f;

	// --- Misc ---
	/** Behavior evaluation period (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler")
	float AZP_EvalInterval = 0.3f;

	/** Max range (UU) a gunshot/impact can wake this crawler. 5000 = 50m. The alert is ALSO gated on a
	 *  clear sightline (same room) — it never travels through a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler")
	float AZP_GunshotAlertRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler")
	bool bAZP_AutoInitialize = true;

	UPROPERTY(BlueprintReadOnly, Category = "Crawler|State")
	ECrawlerState CurrentState = ECrawlerState::Patrol;

	/** Fired on Patrol<->Attack transitions — Blueprint hook for animation / VFX. */
	UPROPERTY(BlueprintAssignable, Category = "Crawler|Events")
	FOnCrawlerStateChanged OnStateChanged;

	// --- API ---
	UFUNCTION(BlueprintCallable, Category = "Crawler")
	void InitializeBehavior();

	/** Commit to a target now — drop/launch if perched, else hunt it on the ground. */
	UFUNCTION(BlueprintCallable, Category = "Crawler")
	void StartHunt(AActor* Target);

	/** A gunshot was heard near this crawler. */
	void OnHearGunshot(const FVector& NoiseLocation, AActor* Shooter);

	/** Alert every crawler within Radius of Location. Called from weapon fire. */
	static void BroadcastGunshot(UWorld* World, const FVector& Location, float Radius, AActor* Shooter);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void EvaluateBehavior();
	void SetState(ECrawlerState NewState);

	// Perch / spawn
	bool TraceSpawnWall(FVector& OutWallPoint, FVector& OutWallNormal) const;
	bool ClingToSpawnWall();

	// Perception
	bool GetPlayerExposure(APawn*& OutPlayer, float& OutDist) const; // eye->crawler clear, walls block
	bool HasLOSTo(const AActor* Target) const;                       // crawler->target clear, walls block
	bool IsPlayerLookingAtMe(const APawn* Player) const;             // gaze cone + exposure

	// Attacks
	void DropToGround();   // silent: release wall, become a ground hunter
	void LaunchAttack();   // ballistic pounce
	void GroundSlam();     // melee in range
	void ApplyAttackDamage();

	void SetSpeed(float Speed);
	void UpdateLurkAudio(float PlayerDist, bool bPerchedAndUnnoticed);

	UFUNCTION()
	void OnOwnerHealthChanged(float NewHealth, float MaxHealth, float DamageAmount);

	/** Registry — gunshot broadcast bypasses FindComponentByClass (CoreRedirect rename issue). */
	static TArray<UZP_CrawlerBehaviorComponent*> AllCrawlers;

	UPROPERTY() TObjectPtr<UZP_CrawlerMovementComponent> CrawlerCMC;
	UPROPERTY() TObjectPtr<UZP_EnemyAudioComponent> EnemyAudio;
	UPROPERTY() TObjectPtr<AActor> ChaseTargetActor;

	bool bHasLeftPerch = false;   // latches on first drop/launch — no re-perch
	float GazeStareTimer = 0.f;
	double LastGazeAlertTime = -1000.0;
	double LastAttackTime = -1000.0;
	bool bAlerted = false;        // played the "noticed you" alert this engagement
	bool bFirstStrike = true;     // first ground slam of an engagement uses the lunge vocal

	// Lurk audio
	float LurkTimer = 0.f;
	float LurkInterval = 8.f;
	bool bWasInLurkRange = false;
	bool bLurkInit = false;

	FVector HomeLocation = FVector::ZeroVector;
	FTimerHandle EvalTimer;
};
