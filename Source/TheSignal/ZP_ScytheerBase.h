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
#include "ZP_ScytheerBase.generated.h"

class UAnimSequence;
class UAnimSingleNodeInstance;
class UZP_HealthComponent;
class UDamageType;
class UPrimitiveComponent;
class USoundBase;
class AController;
class AAIController;

UENUM(BlueprintType)
enum class EScytheerState : uint8
{
	Wander,  // roaming the navmesh slowly
	Alert,   // just noticed the player — holds idle pose + plays alert SFX
	Chase,   // running toward the player
	Attack,  // mid-swipe
	Hit,     // shot — brief flinch
	Die      // dead, holds final frame as corpse
};

UCLASS()
class THESIGNAL_API AZP_ScytheerBase : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_ScytheerBase();

	// ── Detection ──────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float DetectionRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float LoseSightTime = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Detect")
	float GiveUpRange = 3000.f;

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

	// ── Movement (drive CharacterMovement) ─────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float WanderSpeed = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float ChaseSpeed = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scytheer|Move")
	float CombatTurnRate = 300.f;

	// ── Wander ─────────────────────────────────────────────────────
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
	double LastAttackTime = -1000.0;
	double AttackStartTime = 0.0;
	bool bAttackHitFired = false;
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
	float Frame2Time(int32 Frame) const;
	APawn* GetPlayer() const;
	bool HasLOS(const AActor* Target) const;
	void PickNewWanderPoint();
	void SetMaxWalkSpeed(float Speed);
	void FaceTargetSmooth(float DeltaTime);

	UFUNCTION()
	void OnOwnerDied();

	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* FHitComp, FName BoneName, FVector ShotDir, const UDamageType* DamageType, AActor* DamageCauser);
};
