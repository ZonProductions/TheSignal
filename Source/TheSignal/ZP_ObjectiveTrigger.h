// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_ObjectiveTrigger
 *
 * Purpose: Drop-in level volume that drives the campaign ObjectiveSubsystem when the player walks in.
 *          On first overlap it performs one configurable action — set a progression flag, or
 *          complete / start an objective or sub-objective. Used for "reach a place" beats such as
 *          "Go to the empty floor" (Action = SetFlag, TargetId = the flag a stage requirement gates on).
 *          The ObjectiveSubsystem is GameInstance-scoped, so this works across level travel.
 *
 * Owner Subsystem: UZP_ObjectiveSubsystem
 *
 * Blueprint Extension Points:
 *   - TriggerVolume: size/position the box in the placed instance.
 *   - Action / TargetId: configure what fires per-instance.
 *   - bOneShot: fire once (default) or every entry.
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_ObjectiveTrigger.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class EZP_ObjectiveTriggerAction : uint8
{
	SetFlag              UMETA(DisplayName = "Set Flag"),
	CompleteSubObjective UMETA(DisplayName = "Complete Sub-Objective"),
	CompleteObjective    UMETA(DisplayName = "Complete Objective"),
	StartObjective       UMETA(DisplayName = "Start Objective")
};

UCLASS(Blueprintable)
class THESIGNAL_API AZP_ObjectiveTrigger : public AActor
{
	GENERATED_BODY()

public:
	AZP_ObjectiveTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ObjectiveTrigger")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** What to do on the global ObjectiveSubsystem when the player enters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveTrigger")
	EZP_ObjectiveTriggerAction Action = EZP_ObjectiveTriggerAction::SetFlag;

	/** The flag / objective id / sub-objective id passed to the action (e.g. "reached_empty_floor"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveTrigger")
	FName TargetId = NAME_None;

	/** Fire only once, then ignore further overlaps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveTrigger")
	bool bOneShot = true;

	/** Optional gate: only fire while this main objective is active (None = no gate). Stops an
	 *  "arrival" trigger firing before the player is actually on that leg of the campaign. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ObjectiveTrigger")
	FName RequiresActiveObjective = NAME_None;

protected:
	virtual void BeginPlay() override;

private:
	bool bFired = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void Fire();
};
