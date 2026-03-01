// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_GraceCharacter
 *
 * Purpose: First-person character for Grace Owens. Thin shell that owns
 *          components and routes input. All gameplay logic lives in
 *          UZP_GraceGameplayComponent, all Kinemation logic in
 *          UZP_KinemationComponent.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - MovementConfig DataAsset (propagated to GameplayComp).
 *   - WeaponClass (propagated to KinemationComp).
 *   - Input actions set via EditDefaultsOnly (configured in BP child).
 *   - OnInteract BlueprintImplementableEvent for interaction logic.
 *
 * Dependencies:
 *   - EnhancedInput
 *   - UZP_GraceGameplayComponent
 *   - UZP_KinemationComponent
 *   - UZP_GraceMovementConfig
 */

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ZP_GraceCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequenceBase;
class UInputAction;
class UMaterialInterface;
class UZP_GraceMovementConfig;
class UZP_GraceGameplayComponent;
class UZP_KinemationComponent;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_GraceCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZP_GraceCharacter();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** Visible first-person arms mesh. Attached to capsule like reference BP_TacticalShooterCharacter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USkeletalMeshComponent> PlayerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	TObjectPtr<UZP_GraceGameplayComponent> GameplayComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kinemation")
	TObjectPtr<UZP_KinemationComponent> KinemationComp;

	// --- Configuration (propagated to components in PostInitializeComponents) ---

	/** DataAsset with all movement tuning values. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UZP_GraceMovementConfig> MovementConfig;

	/** Blueprint class of weapon to spawn. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Weapon")
	TSubclassOf<AActor> WeaponClass;

	/** Decal materials for bullet impacts. Set in BP child via Python. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Kinemation|Hitscan")
	TArray<TSoftObjectPtr<UMaterialInterface>> BulletDecalMaterials;

	/** Skeletal mesh for hidden locomotion Mesh. Set in BP child. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<USkeletalMesh> LocomotionSkeletalMesh;

	/** Idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> IdleAnimation;

	/** Walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> WalkAnimation;

	/** Run animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> RunAnimation;

	/** Crouch idle animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchIdleAnimation;

	/** Crouch walk animation for hidden locomotion mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Locomotion")
	TObjectPtr<UAnimSequenceBase> CrouchWalkAnimation;

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

	/** Q key — lean peek around cover (camera only, no weapon aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> PeekAction;

	/** RMB — aim down sights. Auto-peeks with weapon when near cover. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Tactical")
	TObjectPtr<UInputAction> ReloadAction;

	// --- BP Interface Compatibility ---
	// These properties are synced from components after init so that
	// BPI_TacticalShooterCharacter interface function implementations
	// (GetPrimaryWeapon, GetMainWeapon) can still read them as variables.
	// Will be removed when BP_GraceCharacter is deprecated.

	UPROPERTY(BlueprintReadOnly, Category = "Kinemation|Weapon")
	TObjectPtr<AActor> ActiveWeapon;

	// --- Interaction ---

	/** Override in Blueprint for interaction logic. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnInteract(AActor* InteractTarget);

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

private:
	// --- Input Handlers ---
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_SprintStarted(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Jump(const FInputActionValue& Value);
	void Input_Interact(const FInputActionValue& Value);
	void Input_CrouchStarted(const FInputActionValue& Value);
	void Input_CrouchCompleted(const FInputActionValue& Value);
	void Input_PeekStarted(const FInputActionValue& Value);
	void Input_PeekCompleted(const FInputActionValue& Value);
	void Input_AimStarted(const FInputActionValue& Value);
	void Input_AimCompleted(const FInputActionValue& Value);
	void Input_FireStarted(const FInputActionValue& Value);
	void Input_FireCompleted(const FInputActionValue& Value);
	void Input_ReloadStarted(const FInputActionValue& Value);
};
