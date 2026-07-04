// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_InteractDoor
 *
 * Purpose: Lightweight interactable door trigger. Place near any door actor
 *          in the level and link AZP_DoorActor to it. Player presses E to
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
#include "ZP_SFXStatics.h"
#include "ZP_InteractDoor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EZP_InteractDoorMode : uint8
{
	Rotate,   // Hinged door — rotates AZP_DoorActor yaw by AZP_OpenAngle
	Slide     // Sliding door — translates AZP_DoorActor by AZP_SlideOffset
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
	 * (one actor = mesh + interaction). Used when AZP_DoorActor is left empty —
	 * ideal for a reusable BP you can drag/copy/paste freely. If AZP_DoorActor IS
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
	TObjectPtr<AActor> AZP_DoorActor;

	/** How the door opens: Rotate (hinged) or Slide (translate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EZP_InteractDoorMode AZP_OpenMode = EZP_InteractDoorMode::Rotate;

	/** Yaw rotation when open (degrees). Only used in Rotate mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "AZP_OpenMode == EZP_InteractDoorMode::Rotate"))
	float AZP_OpenAngle = 90.f;

	/** Relative offset when open. Only used in Slide mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (EditCondition = "AZP_OpenMode == EZP_InteractDoorMode::Slide"))
	FVector AZP_SlideOffset = FVector(0.f, 150.f, 0.f);

	/** FInterpTo speed for smooth open/close animation (the default door feel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float AZP_InterpSpeed = 4.f;

	/** EXACT seconds a full open/close takes, eased. 0 = OFF (default — door keeps the normal
	 *  AZP_InterpSpeed feel). Set per instance only on doors that need a specific time, e.g. a
	 *  big hangar door at 6-8s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door", meta = (ClampMin = "0.0"))
	float AZP_OpenDuration = 0.f;

	/** Distance (UU) within which a co-located StaticMeshActor is treated as the door's frame and gets its collision disabled at BeginPlay (BigCompany pack frames block the doorway); level-dependent, a designer may need to widen it for offset frame pivots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Setup")
	float AZP_FrameCollisionSilenceRadius = 10.f;

	/** If true, door cannot be opened until Unlock() is called (e.g. by a card reader). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bAZP_Locked = false;

	/** Player-facing interaction prompt shown when the door is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|UI")
	FText AZP_LockedPromptText = FText::FromString(TEXT("Locked"));

	/** Sound played at the door each time it STARTS opening — set per instance in the level
	 *  (hangar rumble, hydraulic hiss, ...; any SoundWave/Cue/MetaSound). Routed through the
	 *  SFXStatics carry model. NOT played when a save-load applies the already-open end state.
	 *  None = silent. (Exact opening TIME = AZP_OpenDuration above.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	TObjectPtr<USoundBase> AZP_OpenSound;

	/** How far AZP_OpenSound carries. Room = normal interior door. Far = heard across the
	 *  facility — use for objective doors that open remotely (the player is usually rooms away
	 *  when the flag completes and should still hear it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Audio")
	EZP_SFXCarry AZP_OpenSoundCarry = EZP_SFXCarry::Room;

	/** Unlock this door. Called by AZP_CardReaderPanel or other systems. */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Unlock();

	/** Objective/flag id that auto-opens this door when it completes (e.g. FUSE_BOX: fuses
	 *  deposited -> power restored -> door opens). Matches EITHER a progression flag OR a main
	 *  objective id. NAME_None = off. If the id is already set when the level loads, the door
	 *  starts in its OPEN pose instantly (end-state, no replay swing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Objective")
	FName AZP_ObjectiveOverride = NAME_None;

	/** Unlock + open. bInstant snaps straight to the open pose (save-load end-state). */
	UFUNCTION(BlueprintCallable, Category = "Door")
	void OpenDoor(bool bInstant = false);

	// --- Trace-based door lookup ---

	/** Given a AZP_DoorActor mesh, find the InteractDoor trigger that owns it. */
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

	/** True when moving our own DoorMesh (no external AZP_DoorActor linked). */
	bool bSelfContained = false;

	/** Whether there is anything to move (external actor or built-in mesh). */
	bool HasDoorTarget() const { return AZP_DoorActor != nullptr || bSelfContained; }

	// Unified accessors — operate on the external AZP_DoorActor (world space)
	// or the built-in DoorMesh (relative space) depending on bSelfContained.
	FRotator GetDoorRotation() const;
	void SetDoorRotation(const FRotator& InRot);
	FVector GetDoorLocation() const;
	void SetDoorLocation(const FVector& InLoc);

	/** Maps AZP_DoorActor mesh → owning InteractDoor trigger for trace-based lookup. */
	static TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<AZP_InteractDoor>> DoorActorMap;

	// Rotate mode
	FRotator ClosedRotation;
	FRotator OpenRotation;

	// Slide mode
	FVector ClosedLocation;
	FVector OpenLocation;

	/** Bind to the objective subsystem when AZP_ObjectiveOverride is set (or apply the end
	 *  state instantly when the id already completed before this level loaded). */
	void InitObjectiveOverride();

	/** Play AZP_OpenSound attached to whichever mesh actually moves. */
	void PlayOpenSound();

	/** Begin an open/close animation from the CURRENT pose (captures the fixed-duration start). */
	void StartDoorAnimation();

	// Fixed-duration animation state (used when AZP_OpenDuration > 0)
	float AnimElapsed = 0.f;
	FRotator AnimStartRotation = FRotator::ZeroRotator;
	FVector AnimStartLocation = FVector::ZeroVector;

	UFUNCTION()
	void OnObjectiveOverrideFired(FName Id);

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
