// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_Elevator
 *
 * Purpose: In-map vertical elevator car. Smoothly translates its platform between relative-Z
 *          "stops" within the CURRENT map (no level load) and carries any character standing on
 *          it via UE's native moving-base support. Driven by an AZP_TransitPanel destination of
 *          type InMapElevator (panel->AZP_LinkedElevator->MoveToRelativeZ). Analogous to
 *          AZP_LockableDoor (smooth FInterp Tick) but vertical + rider-carrying. The player does
 *          NOT interact with this actor directly — only through the transit panel console.
 *
 * Owner Subsystem: Transit
 *
 * Blueprint Extension Points:
 *   - PlatformMesh: set the SM_Elevator car mesh in the BP child (Movable, BlockAll).
 *   - AZP_MoveSpeed: travel speed (UU/s).
 *   - AZP_MoveSound / AZP_ArriveSound (+ carry/volume knobs): travel loop + parking beep, routed
 *     through the SFXStatics carry model so distance/occlusion do the filtering.
 *   - OnElevatorArrived / OnElevatorDeparted: delegates for door/audio/VFX hooks.
 *
 * Dependencies: UZP_SFXStatics (audio). Rider carrying relies on the engine's
 *               based-movement (CharacterMovementComponent UpdateBasedMovement), so the platform
 *               must remain Movable with blocking collision.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_SFXStatics.h"
#include "ZP_Elevator.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UAudioComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElevatorArrived, float, RelativeZ);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElevatorDeparted, float, FromRelativeZ);

UCLASS(Blueprintable)
class THESIGNAL_API AZP_Elevator : public AActor
{
	GENERATED_BODY()

public:
	AZP_Elevator();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	TObjectPtr<USceneComponent> ElevatorRoot;

	/** The moving car. Set the SM_Elevator mesh in the BP child. Must be Movable + blocking so the
	 *  rider stands on it and the engine treats it as a movement base. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Elevator")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	// --- Config ---

	/** Constant travel speed in UU/s. Real-elevator feel ~150-250. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator")
	float AZP_MoveSpeed = 200.f;

	/** Snap tolerance (UU) for "arrived". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator")
	float AZP_ArriveTolerance = 1.f;

	// --- Audio (routed through UZP_SFXStatics so space/attenuation/occlusion do the filtering) ---

	/** Looping travel sound — starts when the car starts moving, fades out on arrival.
	 *  Defaults to SFX_Elevator_Move. None = silent travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	TObjectPtr<USoundBase> AZP_MoveSound;

	/** How far the travel sound carries. Close (~30 m max) = clearly audible riding the car or
	 *  standing near the shaft, gone across the map. Bump to Room if it should carry further. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	EZP_SFXCarry AZP_MoveSoundCarry = EZP_SFXCarry::Close;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	float AZP_MoveSoundVolume = 1.f;

	/** Seconds the travel loop fades out after the car parks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	float AZP_MoveSoundFadeOut = 0.4f;

	/** One-shot played at the car every time it parks at a stop. Defaults to SFX_Elevator_Beep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	TObjectPtr<USoundBase> AZP_ArriveSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	EZP_SFXCarry AZP_ArriveSoundCarry = EZP_SFXCarry::Close;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator|Audio")
	float AZP_ArriveSoundVolume = 1.f;

	// --- Delegates ---

	/** Fired when the car reaches a stop (param = the relative Z it arrived at). */
	UPROPERTY(BlueprintAssignable, Category = "Elevator")
	FOnElevatorArrived OnElevatorArrived;

	/** Fired the moment the car STARTS moving (param = the relative Z it left from). Hooks:
	 *  call-button lights back to idle, landing doors close behind the departing car. */
	UPROPERTY(BlueprintAssignable, Category = "Elevator")
	FOnElevatorDeparted OnElevatorDeparted;

	// --- API ---

	/** Travel to RelativeZ measured from the car's BeginPlay origin (0 = start floor, +400 = up). */
	UFUNCTION(BlueprintCallable, Category = "Elevator")
	void MoveToRelativeZ(float RelativeZ);

	UFUNCTION(BlueprintPure, Category = "Elevator")
	bool IsMoving() const { return bMoving; }

	/** Current height above the BeginPlay origin (UU). */
	UFUNCTION(BlueprintPure, Category = "Elevator")
	float GetCurrentRelativeZ() const;

	/** World Z captured at BeginPlay (the car's starting pivot height). Used to convert an
	 *  AZP_TransitLocation's world Z into a relative move: relativeZ = location.Z - GetOriginZ(). */
	UFUNCTION(BlueprintPure, Category = "Elevator")
	float GetOriginZ() const { return OriginLocation.Z; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** World location captured at BeginPlay — all stops are relative to this. */
	FVector OriginLocation = FVector::ZeroVector;

	/** World location the car is travelling toward. */
	FVector TargetLocation = FVector::ZeroVector;

	bool bMoving = false;

	/** Live travel-loop audio while the car moves (faded out on arrival). */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MoveAudioComp;

	/** Shared arrival path: stop the travel loop, play the parking beep, broadcast. */
	void HandleArrived(float RelativeZ);
};
