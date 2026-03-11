// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_GraceMovementConfig
 *
 * Purpose: DataAsset holding ALL movement tuning values for Grace.
 *          No magic numbers in C++ — everything lives here.
 *          Create instances in-editor: DA_GraceMovement_Default, etc.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - All values EditAnywhere for DataAsset instance tuning.
 *   - ZP_GraceCharacter reads these at BeginPlay.
 *
 * Dependencies:
 *   - None
 */

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZP_GraceMovementConfig.generated.h"

UCLASS(BlueprintType)
class THESIGNAL_API UZP_GraceMovementConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- Capsule / Mesh ---

	/** Capsule collision radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capsule")
	float CapsuleRadius = 55.0f;

	/** Capsule half-height (cm). Average male, slightly hunched from anxiety. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capsule")
	float CapsuleHalfHeight = 88.0f;

	/** Crouched capsule half-height (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capsule")
	float CrouchedHalfHeight = 44.0f;

	/** PlayerMesh Z offset from capsule origin (cm). Lines up mesh feet with capsule bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Capsule")
	float PlayerMeshOffsetZ = -90.0f;

	// --- Walk / Sprint ---

	/** Base walk speed. Grace is not a soldier — keep this grounded. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Walk")
	float WalkSpeed = 115.0f;

	/** Sprint speed. Panicked, not trained. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint")
	float SprintSpeed = 205.0f;

	/** Braking deceleration when walking. Higher = stops faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Walk")
	float BrakingDeceleration = 1400.0f;

	/** Max acceleration. Lower = sluggish starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Walk")
	float MaxAcceleration = 1200.0f;

	/** Ground friction. Higher = more grip. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Walk")
	float GroundFriction = 6.0f;

	// --- Stamina ---

	/** Maximum stamina pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Stamina")
	float MaxStamina = 100.0f;

	/** Stamina drain per second while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaDrainRate = 10.0f;

	/** Stamina regen per second while not sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaRegenRate = 15.0f;

	/** Seconds after stopping sprint before regen begins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Stamina")
	float StaminaRegenDelay = 1.5f;

	// --- Head Bob ---

	/** Head bob frequency (cycles per second). Lower = more deliberate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float HeadBobFrequency = 1.6f;

	/** Head bob vertical amplitude in cm. How much the view rises/falls per step. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float HeadBobVerticalAmplitude = 0.8f;

	/** Head bob horizontal sway amplitude in cm. Side-to-side weight shift. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float HeadBobHorizontalAmplitude = 0.4f;

	/** Sprint multiplier for head bob frequency. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float SprintBobFrequencyMultiplier = 1.4f;

	/** Sprint multiplier for head bob amplitude. Grace panics — bob gets worse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float SprintBobAmplitudeMultiplier = 1.5f;

	/** Interpolation speed when returning camera to rest rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|HeadBob")
	float HeadBobReturnSpeed = 6.0f;

	// --- Camera Sway ---

	/** Idle camera sway frequency. Anxious breathing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CameraSway")
	float IdleSwayFrequency = 0.4f;

	/** Idle camera sway amplitude in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|CameraSway")
	float IdleSwayAmplitude = 0.3f;

	// --- Crouch ---

	/** Walk speed while crouched (cm/s). Slow, deliberate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	float CrouchWalkSpeed = 57.0f;

	/** Interpolation speed for camera height when entering/exiting crouch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	float CrouchCameraInterpSpeed = 8.0f;

	// --- Jump ---

	/** Jump velocity. 0 = no jumping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	float JumpZVelocity = 300.0f;

	/** Air control. 0 = no air steering, 1 = full. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
	float AirControl = 0.15f;

	// --- Interaction ---

	/** Max range for interaction line trace (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionTraceRange = 250.0f;

	// --- Camera ---

	/** Camera height offset from capsule center (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float CameraHeightOffset = 64.0f;

	/** Default FOV. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultFOV = 90.0f;

	/** FOV when aiming down sights. Lower = more zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|ADS")
	float AdsFOV = 65.0f;

	/** Interpolation speed for FOV transitions in/out of ADS. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|ADS")
	float AdsFOVInterpSpeed = 10.0f;

	// --- Peek ---

	/** Lateral camera offset (cm) toward the opening side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekLateralOffset = 25.0f;

	/** Forward camera offset (cm) — head past corner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekForwardOffset = 8.0f;

	/** Camera roll angle (degrees) toward peek direction. Lean feel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekRollAngle = 3.0f;

	/** Interpolation speed when entering peek. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekInterpSpeed = 8.0f;

	/** Interpolation speed when exiting peek. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekReturnInterpSpeed = 6.0f;

	/** Sphere trace range (cm) for wall detection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekWallDetectionRange = 180.0f;

	/** Sphere trace radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekTraceRadius = 12.0f;

	/** Minimum traces that must hit per side to count as wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	int32 PeekWallHitThreshold = 2;

	/** Half-angle (degrees) of the 3-ray fan relative to perpendicular. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekTraceFanHalfAngle = 75.0f;

	/** Head bob amplitude multiplier during peek (0 = no bob, 1 = full). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float HeadBobPeekDamping = 0.15f;

	/** Head bob vertical amplitude multiplier during peek. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float HeadBobPeekVerticalDamping = 0.5f;

	/** Max angle (degrees) from vertical a surface can be and still count as wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Peek")
	float PeekMaxWallAngleFromVertical = 20.0f;
};
