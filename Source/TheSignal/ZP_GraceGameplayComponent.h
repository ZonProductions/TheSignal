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

	/** GASP gait state: 0=Walk, 1=Run, 2=Sprint. Matches E_Gait enum order.
	 *  Read by BP_GraceCharacter EventGraph → written to BP Gait variable for AnimBP. */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|GASP")
	uint8 GASPGait = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float CurrentStamina = 100.0f;

	/** Whether to use rotational head bob (pitch/roll). Grace's anxious eye movement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|HeadBob")
	bool bUseBuiltInHeadBob = true;

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

	/** Apply MovementConfig values to the owner's CharacterMovementComponent and camera. */
	UFUNCTION(BlueprintCallable, Category = "Config")
	void ApplyMovementConfig();

	/** Head bob positional offsets (read by external systems if needed). */
	float GetHeadBobOffsetY() const { return HeadBobOffsetY; }
	float GetHeadBobOffsetZ() const { return HeadBobOffsetZ; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// --- Cached References ---

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CachedMovement;

	UPROPERTY()
	TObjectPtr<UZP_EventBroadcaster> EventBroadcaster;

	// --- Head Bob (positional — vertical + horizontal sway) ---
	float HeadBobTimer = 0.0f;
	float BaseCameraZ = 0.0f;
	float HeadBobOffsetY = 0.0f;
	float HeadBobOffsetZ = 0.0f;
	void UpdateHeadBob(float DeltaTime);

	// --- Mesh Peek (gun follows camera) ---
	/** Cached PlayerMesh (camera's attach parent) for applying peek at mesh level. */
	UPROPERTY()
	TObjectPtr<USceneComponent> CachedMeshComponent;

	/** PlayerMesh base relative location (e.g. 0,0,-90), cached in BeginPlay. */
	FVector CachedMeshBaseLocation = FVector::ZeroVector;

	// --- Stamina ---
	float StaminaRegenTimer = 0.0f;
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
