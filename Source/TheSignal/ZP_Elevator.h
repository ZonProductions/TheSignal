// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_Elevator
 *
 * Purpose: In-map vertical elevator car. Smoothly translates its platform between relative-Z
 *          "stops" within the CURRENT map (no level load) and carries any character standing on
 *          it via UE's native moving-base support. Driven by an AZP_TransitPanel destination of
 *          type InMapElevator (panel->LinkedElevator->MoveToRelativeZ). Analogous to
 *          AZP_LockableDoor (smooth FInterp Tick) but vertical + rider-carrying. The player does
 *          NOT interact with this actor directly — only through the transit panel console.
 *
 * Owner Subsystem: Transit
 *
 * Blueprint Extension Points:
 *   - PlatformMesh: set the SM_Elevator car mesh in the BP child (Movable, BlockAll).
 *   - MoveSpeed: travel speed (UU/s).
 *   - OnElevatorArrived: delegate for door/audio/VFX hooks.
 *
 * Dependencies: None — standalone kinematic platform. Rider carrying relies on the engine's
 *               based-movement (CharacterMovementComponent UpdateBasedMovement), so the platform
 *               must remain Movable with blocking collision.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Elevator.generated.h"

class UStaticMeshComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnElevatorArrived, float, RelativeZ);

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
	float MoveSpeed = 200.f;

	/** Snap tolerance (UU) for "arrived". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevator")
	float ArriveTolerance = 1.f;

	// --- Delegate ---

	/** Fired when the car reaches a stop (param = the relative Z it arrived at). */
	UPROPERTY(BlueprintAssignable, Category = "Elevator")
	FOnElevatorArrived OnElevatorArrived;

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
};
