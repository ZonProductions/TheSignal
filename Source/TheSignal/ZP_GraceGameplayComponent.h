// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_GraceGameplayComponent
 *
 * Purpose: Reusable ActorComponent encapsulating Grace's gameplay systems:
 *          stamina, peek/cover, head bob, interaction trace, movement config.
 *          Designed to work on any ACharacter (AZP_GraceCharacter or GASP BP).
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - MovementConfig DataAsset for all tuning values.
 *   - CameraComponent ref (auto-discovered by name "FirstPersonCamera" if not set).
 *   - bUseBuiltInHeadBob flag — disable when Kinemation handles camera.
 *   - bWantsPeek — set true/false from input to control peek behavior.
 *
 * Dependencies:
 *   - UZP_GraceMovementConfig
 *   - UZP_EventBroadcaster (for stamina/sprint/peek events)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_GraceGameplayComponent.generated.h"

class UCameraComponent;
class UCharacterMovementComponent;
class UZP_GraceMovementConfig;
class UZP_EventBroadcaster;

UENUM(BlueprintType)
enum class EZP_PeekDirection : uint8
{
	None  = 0,
	Left  = 1,
	Right = 2
};

UCLASS(ClassGroup=(TheSignal), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_GraceGameplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_GraceGameplayComponent();

	// --- Configuration ---

	/** DataAsset with all movement tuning values. Set via owner or SCS defaults. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> MovementConfig;

	/** Camera to use for peek/bob/interaction trace. Auto-discovered if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
	TObjectPtr<UCameraComponent> CameraComponent;

	// --- Movement State ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	/** Returns the event broadcaster for external binding (HUD, etc). */
	UZP_EventBroadcaster* GetEventBroadcaster() const { return EventBroadcaster; }

	/** GASP gait state: 0=Walk, 1=Run, 2=Sprint. Matches E_Gait enum order.
	 *  Read by BP_GraceCharacter EventGraph → written to BP Gait variable for AnimBP. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|GASP")
	uint8 GASPGait = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentStamina = 100.0f;

	/** Whether to use rotational head bob (pitch/roll). Grace's anxious eye movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|HeadBob")
	bool bUseBuiltInHeadBob = true;

	/** Extra forward (capsule +X) camera clearance, cm, used as a TRANSIENT nudge
	 *  to cover the block/dodge stance lean-IN (when the spine pitches forward and
	 *  the body would otherwise swing toward the lens). Snaps in instantly, then
	 *  eases back to 0 — it is NOT held for the duration of a block, or a sustained
	 *  hold would leave the camera floating forward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float BlockDodgeForwardClearance = 14.0f;

	/** Interp speed for easing the block/dodge forward clearance back out to 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float ForwardClearanceInterpSpeed = 8.0f;

	/** Forward (capsule +X) camera offset. The Operator's FPCamera socket sits ~62cm
	 *  FORWARD of the body (fine when the body was hidden) — this pulls the camera back
	 *  onto Marcus's head so looking down shows chest->feet. Measured/verified live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float CameraExtraForward = -70.0f;

	/** World-up camera offset — sets the camera to Marcus's eye line. Verified live:
	 *  head ~10cm above camera = eye level, body extends below into the down-view. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float CameraExtraHeight = 12.0f;

	/** When false, the Marcus eye-offset above is replaced by the ranged offsets below
	 *  — used while a ranged weapon is up (visible arms are the Kinemation Operator on
	 *  PlayerMesh, a different body). Set by the character. */
	UPROPERTY(BlueprintReadWrite, Category = "Movement|Camera")
	bool bCameraOffsetActive = true;

	/** Camera forward/up offsets used ONLY while a ranged weapon is up — separate from
	 *  the Marcus offsets so you can frame the armed POV (where the gun arms are)
	 *  independently of the unarmed body view. Default 0,0 = camera AT the Operator
	 *  FPCamera socket, i.e. the gun where the Kinemation pack authored it (correct hip
	 *  + ADS distance). The old -70 default pulled the camera off that socket and shoved
	 *  the gun into frame. Tune from 0. Live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float CameraRangedForward = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Camera")
	float CameraRangedHeight = 0.0f;

	/** Extra camera offset during weapon actions (reload/switch/swing/block), set by the
	 *  character each frame (already lerped). Capsule space: +X = forward, +Z = up. Added
	 *  on top of the camera position so the leaning body doesn't clip the lens. */
	UPROPERTY(BlueprintReadWrite, Category = "Movement|Camera")
	FVector WeaponActionCamOffset = FVector::ZeroVector;


	// --- Peek State ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	EZP_PeekDirection CurrentPeekDirection = EZP_PeekDirection::None;

	/** 0 = no peek, 1 = fully peeked. Interpolated. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	float PeekAlpha = 0.0f;

	/** Set to true while peek input (Q) is held. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	bool bWantsPeek = false;

	/** Set to true while ADS input (RMB) is held. Triggers wall-peek + aim when near cover. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Aim")
	bool bWantsAim = false;

	// --- Interaction ---

	/** Current actor under interaction trace (null if nothing). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> CurrentInteractionTarget;

	// --- Public API ---

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();

	/** Spend a flat PERCENT of max stamina (0-100) for a one-shot action (dodge).
	 *  Returns false and spends nothing if the player has less than Percent left.
	 *  On success, resets the regen delay so stamina doesn't immediately tick back. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	bool TryConsumeStaminaPercent(float Percent);

	/** Forward-axis move input (W=+1, S=-1). Set every Input_Move tick; reset to 0 on Completed. */
	void SetForwardInput(float Value) { CurrentForwardInput = Value; }

	/** Apply MovementConfig values to the owner's CharacterMovementComponent and camera. */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void ApplyMovementConfig();

	/** Called by character on crouch start/end. HeightAdjust > 0 entering crouch, < 0 exiting. */
	void OnCrouchHeightChanged(float HeightAdjust);

	/** Head bob positional offsets (read by external systems if needed). */
	float GetHeadBobOffsetY() const { return HeadBobOffsetY; }
	float GetHeadBobOffsetZ() const { return HeadBobOffsetZ; }

	/** Called by the character each frame: true during the short block/dodge
	 *  lean-in window. While true the camera snaps to BlockDodgeForwardClearance of
	 *  forward push; when it goes false the push eases back to 0. */
	void SetForwardClearanceActive(bool bActive) { bForwardClearanceActive = bActive; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// --- Cached References ---

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CachedMovement;

	UPROPERTY()
	TObjectPtr<UZP_EventBroadcaster> EventBroadcaster;

	// --- Crouch Mesh Smoothing ---
	/** Compensates PlayerMesh Z for instant capsule snap. Lerps to 0. */
	float CrouchMeshOffsetZ = 0.0f;

	// --- Head Bob (positional — vertical + horizontal sway) ---
	float HeadBobTimer = 0.0f;
	float BaseCameraZ = 0.0f;
	/** Camera relative X offset (forward of FPCamera bone socket). Captured at
	 *  BeginPlay so peek/bob math preserves the constructor's forward push
	 *  that keeps the camera ahead of the body during spine lean. */
	float BaseCameraX = 0.0f;
	float HeadBobOffsetY = 0.0f;
	float HeadBobOffsetZ = 0.0f;
	void UpdateHeadBob(float DeltaTime);

	// --- Block/dodge forward camera clearance ---
	/** Set by the character via SetForwardClearanceActive each tick. */
	bool bForwardClearanceActive = false;
	/** Eased current clearance (cm) added to the camera's capsule-space forward offset. */
	float CurrentForwardClearance = 0.0f;

	// --- Mesh Peek (gun follows camera) ---
	/** Cached PlayerMesh (camera's attach parent) for applying peek at mesh level. */
	UPROPERTY()
	TObjectPtr<USceneComponent> CachedMeshComponent;

	/** PlayerMesh base relative location (e.g. 0,0,-90), cached in BeginPlay. */
	FVector CachedMeshBaseLocation = FVector::ZeroVector;

	// --- Stamina ---
	float StaminaRegenTimer = 0.0f;
	float CurrentForwardInput = 0.0f;
	float WallStuckTimer = 0.0f;
	void UpdateStamina(float DeltaTime);

	// --- GASP State ---
	void UpdateGASPState();

	// --- Interaction ---
	void UpdateInteractionTrace();

	// --- Peek ---
	EZP_PeekDirection PreviousPeekDirection = EZP_PeekDirection::None;
	EZP_PeekDirection LockedPeekDirection = EZP_PeekDirection::None;
	bool bPeekLocked = false;
	bool bPeekFromAim = false;
	float CurrentPeekRoll = 0.0f;

	/** Cast 5 sphere traces on one side. Returns number of hits on roughly vertical surfaces. */
	int32 TracePeekSide(const FVector& Origin, const FVector& Forward, const FVector& Right, float DirectionSign) const;

	/** Evaluate both sides and return which direction to peek. */
	EZP_PeekDirection DetectPeekDirection() const;

	/** Per-frame peek interpolation and camera offset. Called in TickComponent. */
	void UpdatePeek(float DeltaTime);
};
