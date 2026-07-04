// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_TransitReturn
 *
 * Purpose: Placeable elevator "call" button mounted at a floor. Player overlaps the trigger,
 *          presses E, and the linked elevator car is RECALLED to THIS floor — the floor the
 *          return actor sits on. Unlike AZP_TransitPanel (which rides inside the car and opens a
 *          floor-selection menu), this is a fixed wall button: one press, no menu. It resolves its
 *          target Z from a nearby AZP_TransitLocation (the floor's resting pivot marker) so it stays
 *          correct without hand-tuned numbers. Its interaction volume defaults LARGE/tall so it
 *          covers the whole elevator-shaft opening at this floor with no per-instance tuning.
 *
 * Owner Subsystem: Transit
 *
 * Blueprint Extension Points:
 *   - ButtonMesh: set the call-button mesh in the BP child.
 *   - AZP_LinkedElevator: the car this button recalls.
 *   - AZP_ReturnLocation: explicit stop marker for this floor (optional — auto-finds the nearest
 *     AZP_TransitLocation at BeginPlay when left unset and bAZP_AutoFindNearestLocation is true).
 *   - InteractionVolume: shrink/resize per placement if the default shaft-sized box is too big.
 *
 * Dependencies: IZP_Interactable, AZP_GraceCharacter (interaction + HUD prompt), AZP_Elevator
 *               (MoveToRelativeZ), AZP_TransitLocation (Z target).
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_TransitReturn.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class AZP_Elevator;
class AZP_TransitLocation;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_TransitReturn : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_TransitReturn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<UStaticMeshComponent> ButtonMesh;

	/** Defaults LARGE/tall so it covers the whole shaft opening at this floor. Resize per placement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** The elevator car this button recalls. Required. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Transit")
	TObjectPtr<AZP_Elevator> AZP_LinkedElevator;

	/** The stop marker for THIS floor (target Z the car is recalled to). Leave unset to auto-find the
	 *  nearest AZP_TransitLocation at BeginPlay. If set, this overrides the auto-find. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Transit")
	TObjectPtr<AZP_TransitLocation> AZP_ReturnLocation;

	/** When AZP_ReturnLocation is unset, find and cache the nearest AZP_TransitLocation at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit")
	bool bAZP_AutoFindNearestLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	FText AZP_PromptText = FText::FromString(TEXT("Call Elevator"));

	// --- IZP_Interactable ---
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

	/** Recall the linked elevator to this floor's resolved AZP_TransitLocation Z. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void CallElevatorHere();

protected:
	virtual void BeginPlay() override;

private:
	/** AZP_ReturnLocation if set; otherwise the cached/auto-found nearest AZP_TransitLocation. */
	AZP_TransitLocation* ResolveReturnLocation();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Nearest stop marker found at BeginPlay (used only when AZP_ReturnLocation is unset). */
	TWeakObjectPtr<AZP_TransitLocation> CachedNearestLocation;
};
