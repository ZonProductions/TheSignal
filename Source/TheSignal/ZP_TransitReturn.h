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
 *   - AZP_RequiredObjective: objective/flag gate — button is dead (and IndicatorLight off) until it
 *     completes (e.g. the fuse-box BP_ObjectiveContainer's unlock flag).
 *   - IndicatorLight: dim white when powered, AZP_ArrivedLightColor while the car is parked here.
 *   - AZP_PressSound (+ carry/volume): beep on a successful button press.
 *   - InteractionVolume: shrink/resize per placement if the default shaft-sized box is too big.
 *
 * Dependencies: IZP_Interactable, AZP_GraceCharacter (interaction + HUD prompt), AZP_Elevator
 *               (MoveToRelativeZ + arrive/depart delegates), AZP_TransitLocation (Z target),
 *               UZP_ObjectiveSubsystem (gate), UZP_SFXStatics (audio).
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_SFXStatics.h"
#include "ZP_TransitReturn.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPointLightComponent;
class USoundBase;
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

	/** Call-button indicator. OFF while the AZP_RequiredObjective gate is unmet; dim white once the
	 *  elevator works; AZP_ArrivedLightColor while the car is parked at this floor. Position/intensity
	 *  tunable on the component in the BP child. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transit")
	TObjectPtr<UPointLightComponent> IndicatorLight;

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

	/** Prompt shown while the AZP_RequiredObjective gate is unmet. PLACEHOLDER — author per level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	FText AZP_UnpoweredPromptText = FText::FromString(TEXT("No Power"));

	// --- Objective gate ---

	/** Objective/flag id that powers this call button — matches EITHER a progression flag OR a main
	 *  objective id (e.g. a fuse-box BP_ObjectiveContainer's AZP_ObjectiveFlagOnUnlock). Until it
	 *  completes, the button is dead and IndicatorLight is OFF; the moment it completes the light
	 *  comes on dim white and the button works. NAME_None = no gate, always powered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Objective")
	FName AZP_RequiredObjective = NAME_None;

	// --- Indicator light ---

	/** Idle light color while the elevator works (dim white). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Light")
	FLinearColor AZP_ReadyLightColor = FLinearColor::White;

	/** Light color while the car is parked at THIS floor (back to ready color when it leaves). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Light")
	FLinearColor AZP_ArrivedLightColor = FLinearColor(0.1f, 0.8f, 0.1f);

	/** |car Z − this floor's stop Z| within this margin (UU) counts as "the car is at this floor". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Light")
	float AZP_ArriveZMargin = 100.f;

	// --- Audio ---

	/** Beep on a successful button press (gate met + elevator linked). Defaults to SFX_Elevator_Beep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Audio")
	TObjectPtr<USoundBase> AZP_PressSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Audio")
	EZP_SFXCarry AZP_PressSoundCarry = EZP_SFXCarry::Close;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit|Audio")
	float AZP_PressSoundVolume = 1.f;

	// --- IZP_Interactable ---
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

	/** Recall the linked elevator to this floor's resolved AZP_TransitLocation Z. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void CallElevatorHere();

	/** True when no AZP_RequiredObjective is set, or it has completed (flag or objective). */
	UFUNCTION(BlueprintPure, Category = "Transit")
	bool IsPowered() const { return bPowered; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** AZP_ReturnLocation if set; otherwise the cached/auto-found nearest AZP_TransitLocation. */
	AZP_TransitLocation* ResolveReturnLocation();

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Evaluate the objective gate now; arm on the subsystem's delegates if not yet met. */
	void InitObjectiveGate();

	UFUNCTION()
	void OnObjectiveGateFired(FName Id);

	UFUNCTION()
	void OnLinkedElevatorArrived(float RelativeZ);

	UFUNCTION()
	void OnLinkedElevatorDeparted(float FromRelativeZ);

	/** Apply power + parked state to IndicatorLight (visibility + color). */
	void UpdateIndicatorLight();

	/** True while the car sits within AZP_ArriveZMargin of this floor's stop Z. */
	bool IsCarAtThisFloor();

	bool bPowered = false;
	bool bCarAtThisFloor = false;

	/** Nearest stop marker found at BeginPlay (used only when AZP_ReturnLocation is unset). */
	TWeakObjectPtr<AZP_TransitLocation> CachedNearestLocation;
};
