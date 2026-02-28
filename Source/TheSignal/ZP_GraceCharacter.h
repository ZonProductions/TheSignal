// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GraceCharacter
 *
 * Purpose: First-person character for Grace Owens. Handles movement, camera,
 *          stamina, head bob, input binding, and interaction trace. Movement
 *          values come from UZP_GraceMovementConfig DataAsset — no magic numbers.
 *
 *          Kinemation integration: Blueprint child (BP_GraceCharacter) adds SCS
 *          components and sets the UPROPERTY refs via setter functions. This class
 *          does NOT create Kinemation components — they are Blueprint classes.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - MovementConfig DataAsset for all tuning values.
 *   - Input actions set via EditDefaultsOnly (configured in BP child).
 *   - Kinemation component refs set by BP child at BeginPlay.
 *   - OnInteract BlueprintImplementableEvent for interaction logic.
 *   - bUseBuiltInHeadBob flag — disable when Kinemation handles camera.
 *
 * Dependencies:
 *   - EnhancedInput
 *   - UZP_GraceMovementConfig
 *   - UZP_EventBroadcaster (for stamina/sprint events)
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ZP_GraceCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputAction;
class UZP_GraceMovementConfig;
class UZP_EventBroadcaster;

UENUM(BlueprintType)
enum class EZP_PeekDirection : uint8
{
	None  = 0,
	Left  = 1,
	Right = 2
};

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GraceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_GraceCharacter();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Visible first-person arms mesh. Child of FirstPersonCamera so arms follow the view. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	// --- Configuration ---

	/** DataAsset with all movement tuning values. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> MovementConfig;

	// --- Movement State ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentStamina = 100.0f;

	/** Whether to use the built-in C++ head bob. Disable when Kinemation controls camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|HeadBob")
	bool bUseBuiltInHeadBob = true;

	// --- Peek State ---

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	EZP_PeekDirection CurrentPeekDirection = EZP_PeekDirection::None;

	/** 0 = no peek, 1 = fully peeked. Interpolated. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	float PeekAlpha = 0.0f;

	/** True while RMB is held. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Peek")
	bool bWantsPeek = false;

	// --- Input Actions (set in Blueprint child, e.g. BP_GraceCharacter) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;

	// Kinemation-related actions (bound when Kinemation is integrated)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> ReloadAction;

	// --- Kinemation Camera API ---

	/** Trigger a Kinemation camera shake. Requires TacticalCameraComp to be set. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void TriggerCameraShake(UObject* ShakeData);

	/** Smoothly interpolate FOV to a new target. Requires TacticalCameraComp to be set. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Kinemation")
	void SetTargetFOV(float NewFOV, float InterpSpeed = 5.0f);

	// --- Kinemation Weapon ---

	/** Blueprint class of weapon to spawn at BeginPlay. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** The spawned weapon actor (if any). */
	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	// --- Sprint ---

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();

	// --- Interaction ---

	/** Override in Blueprint for interaction logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteract(AActor* InteractTarget);

	/** Current actor under interaction trace (null if nothing). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> CurrentInteractionTarget;

	// --- Kinemation Component Refs (set by Blueprint child) ---

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalCameraComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> TacticalAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> RecoilAnimComp;

	UPROPERTY(BlueprintReadWrite, Category = "Kinemation")
	TObjectPtr<UActorComponent> IKMotionComp;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// --- Head Bob ---
	float HeadBobTimer = 0.0f;
	float BaseCameraZ = 0.0f;
	float HeadBobOffsetY = 0.0f;
	float HeadBobOffsetZ = 0.0f;
	void UpdateHeadBob(float DeltaTime);

	// --- Stamina ---
	float StaminaRegenTimer = 0.0f;
	void UpdateStamina(float DeltaTime);

	// --- Interaction ---
	void UpdateInteractionTrace();

	// --- Config Application ---
	void ApplyMovementConfig();

	// --- Kinemation ---
	void InitKinemationCamera();
	void InitKinemationAnimation();
	void SpawnAndEquipWeapon();

	// --- Peek ---
	EZP_PeekDirection PreviousPeekDirection = EZP_PeekDirection::None;
	EZP_PeekDirection LockedPeekDirection = EZP_PeekDirection::None;
	bool bPeekLocked = false;
	float CurrentPeekRoll = 0.0f;

	/** Cast 3 sphere traces on one side. Returns number of hits on roughly vertical surfaces. */
	int32 TracePeekSide(const FVector& Origin, const FVector& Forward, const FVector& Right, float DirectionSign) const;

	/** Evaluate both sides and return which direction to peek. */
	EZP_PeekDirection DetectPeekDirection() const;

	/** Per-frame peek interpolation and camera offset. Called in Tick. */
	void UpdatePeek(float DeltaTime);

	// --- Input Handlers ---
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_CrouchStarted(const FInputActionValue& Value);
	void Input_CrouchCompleted(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_ReloadStarted(const FInputActionValue& Value);

	// --- Event Broadcaster Cache ---
	UPROPERTY()
	TObjectPtr<UZP_EventBroadcaster> EventBroadcaster;
};
