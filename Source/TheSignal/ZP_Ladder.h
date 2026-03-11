// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_Ladder
 *
 * Purpose: Climbable ladder actor. Player interacts (E) to mount, then
 *          W/S to climb up/down, Space to dismount.
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - LadderMesh: assign any static mesh (SM_Ladder, pack meshes, etc.)
 *   - ClimbSpeed: tune via DataAsset or instance edit
 *   - BottomAttach / TopAttach: scene components marking mount/dismount points
 *
 * Dependencies:
 *   - IZP_Interactable (player interaction)
 *   - AZP_GraceCharacter (SetCurrentInteractable/ClearCurrentInteractable)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_Ladder.generated.h"

class UBoxComponent;
class UArrowComponent;

UCLASS()
class THESIGNAL_API AZP_Ladder : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_Ladder();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UStaticMeshComponent> LadderMesh;

	/** Where the player stands when mounting from the bottom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> BottomAttachPoint;

	/** Where the player stands after climbing to the top. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<USceneComponent> TopExitPoint;

	/** Direction player faces while climbing (arrow points INTO the ladder). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UArrowComponent> ClimbDirection;

	/** Trigger volume around the ladder for interaction detection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	TObjectPtr<UBoxComponent> InteractionVolume;

	// --- Config ---

	/** Vertical climb speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder|Config")
	float ClimbSpeed = 100.0f;

	/** Bottom Z limit — player can't go below this (world space, computed from BottomAttachPoint). */
	float GetBottomZ() const;

	/** Top Z limit — player exits when reaching this (world space, computed from TopExitPoint). */
	float GetTopZ() const;

	/** World rotation the player should face while climbing. */
	FRotator GetClimbFacingRotation() const;

	/** World location of the bottom attach point. */
	FVector GetBottomAttachLocation() const;

	/** World location of the top exit point. */
	FVector GetTopExitLocation() const;

	// --- IZP_Interactable ---
	FText GetInteractionPrompt_Implementation() override;
	void OnInteract_Implementation(ACharacter* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
