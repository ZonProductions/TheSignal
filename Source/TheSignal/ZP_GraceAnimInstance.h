// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_GraceAnimInstance
 *
 * Purpose: C++ AnimInstance base for Grace's locomotion. Reads movement state
 *          from the owning character every frame via NativeUpdateAnimation().
 *          Blueprint AnimBP (ABP_GraceLocomotion) extends this to drive
 *          blend spaces and state machines without any GASP/PoseSearch dependency.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - All UPROPERTY values available in AnimGraph for blend space / state machine.
 *   - Override NativeUpdateAnimation in BP via BlueprintUpdateAnimation if needed.
 *
 * Dependencies:
 *   - AZP_GraceCharacter (or any ACharacter — reads from CharacterMovementComponent)
 */

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZP_GraceAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class THESIGNAL_API UZP_GraceAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// --- Locomotion State (updated every frame, read by AnimGraph) ---

	/** Ground speed (cm/s). Drives locomotion blend space X axis. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.0f;

	/** Movement direction relative to actor facing (-180 to 180). Drives blend space Y axis. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	/** True when character is falling or jumping. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	/** True when sprint input is held and character has stamina. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	/** True when character is crouching. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching = false;

	/** True when character has meaningful ground velocity. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	/** Gait index: 0=Walk, 1=Run, 2=Sprint. Matches GASPGait from GameplayComponent. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	uint8 Gait = 1;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> CachedCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CachedMovement;
};
