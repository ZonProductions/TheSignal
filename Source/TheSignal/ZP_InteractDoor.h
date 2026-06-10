// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_InteractDoor
 *
 * Purpose: Lightweight interactable door trigger. Place near any door actor
 *          in the level and link DoorActor to it. Player presses E to
 *          toggle open/close. Supports Rotate (hinged) and Slide modes.
 *
 *          Does NOT replace AZP_LockableDoor — that class handles card-reader
 *          locked doors. This is for simple unlocked doors.
 *
 * Owner Subsystem: FacilitySystemsManager
 *
 * Dependencies:
 *   - IZP_Interactable (interaction interface)
 *   - AZP_GraceCharacter (overlap detection)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_InteractDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EZP_InteractDoorMode : uint8
{
	Rotate,   // Hinged door — rotates DoorActor yaw by OpenAngle
	Slide     // Sliding door — translates DoorActor by SlideOffset
};

UCLASS(Blueprintable)
class THESIGNAL_API AZP_InteractDoor : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_InteractDoor();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/**
	 * Built-in door mesh. Assign a mesh here to make a SELF-CONTAINED door
	 * (one actor = mesh + interaction). Used when DoorActor is left empty —
	 * ideal for a reusable BP you can drag/copy/paste freely. If DoorActor IS
	 * set, this stays empty and the external actor is moved instead.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// --- Config ---

	/**
	 * Optional external door actor in the level to move. Leave EMPTY for a
	 * self-contained door (assign DoorMesh instead).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Door")
	TObjectPtr<AActor> DoorActor;

	/** How the door opens: Rotate (hinged) or Slide (translate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EZP_InteractDoorMode OpenMode = EZP_InteractDoorMode::Rotate;

	/** Yaw rotation when open (degrees). Only used in Rotate mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "OpenMode == EZP_InteractDoorMode::Rotate"))
	float OpenAngle = 90.f;

	/** Relative offset when open. Only used in Slide mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "OpenMode == EZP_InteractDoorMode::Slide"))
	FVector SlideOffset = FVector(0.f, 150.f, 0.f);

	/** FInterpTo speed for smooth open/close animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float InterpSpeed = 4.f;

	/** If true, door cannot be opened until Unlock() is called (e.g. by a card reader). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bLocked = false;

	/** Unlock this door. Called by AZP_CardReaderPanel or other systems. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Unlock();

	// --- Trace-based door lookup ---

	/** Given a DoorActor mesh, find the InteractDoor trigger that owns it. */
	static AZP_InteractDoor* FindDoorForActor(AActor* Actor);

	// --- IZP_Interactable ---

	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bIsOpen = false;
	bool bIsAnimating = false;

	/** True when moving our own DoorMesh (no external DoorActor linked). */
	bool bSelfContained = false;

	/** Whether there is anything to move (external actor or built-in mesh). */
	bool HasDoorTarget() const { return DoorActor != nullptr || bSelfContained; }

	// Unified accessors — operate on the external DoorActor (world space)
	// or the built-in DoorMesh (relative space) depending on bSelfContained.
	FRotator GetDoorRotation() const;
	void SetDoorRotation(const FRotator& InRot);
	FVector GetDoorLocation() const;
	void SetDoorLocation(const FVector& InLoc);

	/** Maps DoorActor mesh → owning InteractDoor trigger for trace-based lookup. */
	static TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AZP_InteractDoor>> DoorActorMap;

	// Rotate mode
	FRotator ClosedRotation;
	FRotator OpenRotation;

	// Slide mode
	FVector ClosedLocation;
	FVector OpenLocation;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
