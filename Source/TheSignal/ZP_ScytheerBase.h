// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ScytheerBase
 *
 * Purpose: Ground-roaming Scytheer enemy. Wanders the navmesh at a slow pace; when the player
 *          enters DetectionRange with line of sight, transitions to Alert (plays idle pose +
 *          alert SFX for AlertHoldTime), then Chase (runs toward player). On reach, swings one
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
 *   - SingleAnim: the source clip to slice.
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
	float DetectionRange = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float LoseSightTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float GiveUpRange = 2200.f;

	/** Max navmesh PATH length (UU) that counts as "same geometry". A closed door blocks the
	 *  navmesh, so even a 200-UU straight-line gap reads as unreachable -> no aggro. An open door
	 *  with navmesh through it reads as reachable -> aggro. 1.6x DetectionRange covers a typical
	 *  in-room S-shape but rejects "all the way around the building" paths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float MaxReachablePathLength = 1600.f;

	// ── Damage / Death ─────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float BodyShotDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float HeadShotDamage = 50.f;

	/** Hits above this Z (above the actor pivot) count as headshots. Capsule hits carry no bone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Damage")
	float HeadshotMinZ = 40.f;

	// ── Attack ─────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AttackRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AttackCooldown = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float AttackDamage = 20.f;

	/** Min seconds between hit-react flinches. Without a CD the Scytheer can be perma-stunlocked
	 *  by sustained fire — every bullet triggers a fresh Hit state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Attack")
	float HitReactCooldown = 1.0f;

	/** Z above the patrol spline's ground point that counts as "on the wall" — controls whether
	 *  aggro transitions through WallDescent or straight into Alert. 100 UU keeps the climb-and-
	 *  back-down on the floor portion of the spline from falsely triggering descent every time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float OnWallZThreshold = 100.f;

	// ── Movement (drive CharacterMovement) ─────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float WanderSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float ChaseSpeed = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float CombatTurnRate = 300.f;

	// ── Patrol (optional designer-guided path) ─────────────────────
	/** If set, Wander walks back and forth along this spline instead of picking random navmesh points.
	 *  Drop a ZP_ScytheerClimbPath actor in the level, shape its spline, then assign here. The path's
	 *  points must sit on the navmesh for the AI to walk between them. Chase still uses navmesh; on
	 *  losing the player the Scytheer rejoins the nearest point on the path and resumes patrol. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	TObjectPtr<AZP_ScytheerClimbPath> PatrolPath;

	/** Distance between consecutive patrol waypoints along the spline (UU). Smaller = the AI re-paths
	 *  more often (smoother curves but more CPU); larger = it cuts corners on tight bends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float PatrolStep = 200.f;

	/** Max rotational speed (deg/s) the body uses while patrolling. Without a cap, reversing
	 *  direction at the spline endpoints flips the body 180° in a single frame (snap). 360 = the
	 *  reversal takes 0.5s; lower = more deliberate-looking turns. Set very high (e.g. 9999) for
	 *  the original instant-flip behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Patrol")
	float PatrolTurnRate = 360.f;

	// ── Wander (random-roam fallback when PatrolPath is unset) ─────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float WanderRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float PauseMin = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Wander")
	float PauseMax = 3.5f;

	// ── Alert (plays after first detection, before Chase) ──────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Alert")
	float AlertHoldTime = 2.0f;

	// ── Audio placeholders ─────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AlertSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundBase> LurkSound;

	/** Spatial attenuation (distance falloff + occlusion). Defaults to SA_EnemyVoice so wall
	 *  occlusion mutes Scytheer SFX from the next room. Without this, PlaySoundAtLocation plays
	 *  unspatialized 2D audio audible everywhere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Audio")
	TObjectPtr<USoundAttenuation> AudioAttenuation;

	// ── Animation source ───────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	TObjectPtr<UAnimSequence> SingleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 WalkStartFrame = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 WalkEndFrame = 33;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 RunStartFrame = 35;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 RunEndFrame = 53;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 IdleStartFrame = 55;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 IdleEndFrame = 190;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack1StartFrame = 192;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack1EndFrame = 222;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack2StartFrame = 223;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack2EndFrame = 288;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack3StartFrame = 291;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 Attack3EndFrame = 321;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 HitStartFrame = 323;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 HitEndFrame = 353;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 DieStartFrame = 354;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Anim")
	int32 DieEndFrame = 500;

	UPROPERTY(BlueprintReadOnly, Category = "Scytheer|State")
	EScytheerState State = EScytheerState::Wander;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bDead = false;
	bool bAggro = false;
	float LostSightTimer = 0.f;
	float PauseTimer = 0.f;
	bool bWanderMoving = false;
	FVector WanderDest = FVector::ZeroVector;
	float PatrolDistance = 0.f;
	int32 PatrolDir = 1;
	/** Cached so we can restore CharacterMovement.MovementMode when leaving spline-direct patrol. */
	uint8 PreSplineMovementMode = 1; // MOVE_Walking
	double LastAttackTime = -1000.0;
	double AttackStartTime = 0.0;
	bool bAttackHitFired = false;
	double LastHitReactTime = -1000.0;
	/** While true, the Hit state holds (stays staggered) until StaggerHandle fires instead of
	 *  auto-returning to Chase/Wander when the flinch clip ends. Set by ReceiveStagger. */
	bool bStaggerHold = false;
	FTimerHandle StaggerHandle;
	float ChaseStuckTimer = 0.f;
	FVector LastChaseStuckLoc = FVector::ZeroVector;
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
	float Frame2Time(int32 Frame) const;
	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;
	/** Is the player reachable on the same connected navmesh region within MaxReachablePathLength?
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
