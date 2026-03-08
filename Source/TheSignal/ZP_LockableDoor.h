// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_LockableDoor
 *
 * Purpose: A lockable barrier that starts Locked, can be Unlocked by a card
 *          reader panel, and smoothly opens via rotation (hinged door) or
 *          translation (sliding fence/gate). Player does NOT interact with
 *          this actor directly — only through the card reader panel.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Blueprint Extension Points:
 *   - DoorMesh / FrameMesh: set static meshes in BP child.
 *   - OpenMode: Rotate (hinged door) or Slide (chain fence / gate).
 *   - OpenAngle / SlideOffset / OpenInterpSpeed: tunable per-instance.
 *   - OnDoorStateChanged: delegate for audio/VFX hooks.
 *
 * Dependencies: None — standalone actor.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_LockableDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EZP_DoorState : uint8
{
	Locked,
	Closed,
	Opening,
	Open
};

UENUM(BlueprintType)
enum class EZP_DoorOpenMode : uint8
{
	Rotate,   // Hinged door — rotates DoorPivot yaw
	Slide     // Sliding fence/gate — translates DoorPivot along SlideOffset
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDoorStateChanged, EZP_DoorState, NewState);

UCLASS(Blueprintable)
class THESIGNAL_API AZP_LockableDoor : public AActor
{
	GENERATED_BODY()

public:
	AZP_LockableDoor();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> DoorRoot;

	/** Rotation pivot — door mesh attaches here. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> DoorPivot;

	/** The moving door panel. Set mesh in BP child. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Non-moving door frame. Set mesh in BP child. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> FrameMesh;

	// --- Config ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	EZP_DoorState InitialState = EZP_DoorState::Locked;

	/** How the door opens: Rotate (hinged) or Slide (translate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	EZP_DoorOpenMode OpenMode = EZP_DoorOpenMode::Rotate;

	/** Target yaw rotation when fully open (degrees). Only used in Rotate mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "OpenMode == EZP_DoorOpenMode::Rotate"))
	float OpenAngle = 90.f;

	/** Relative offset to slide to when fully open. Only used in Slide mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "OpenMode == EZP_DoorOpenMode::Slide"))
	FVector SlideOffset = FVector(0.f, 300.f, 0.f);

	/** FInterpTo speed for door movement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float OpenInterpSpeed = 3.f;

	// --- Delegate ---

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnDoorStateChanged OnDoorStateChanged;

	// --- API ---

	/** Transitions Locked -> Closed. No-op if not Locked. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Unlock();

	/** Transitions Closed -> Opening. No-op if not Closed. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void OpenDoor();

	UFUNCTION(BlueprintPure, Category = "Door")
	EZP_DoorState GetDoorState() const { return CurrentState; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	EZP_DoorState CurrentState = EZP_DoorState::Locked;

	// Rotate mode
	float TargetYaw = 0.f;
	float InitialYaw = 0.f;

	// Slide mode
	FVector InitialLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;

	void SetState(EZP_DoorState NewState);
};
