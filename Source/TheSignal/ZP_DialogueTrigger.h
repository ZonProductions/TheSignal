// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_DialogueTrigger
 *
 * Purpose: Placeable actor with a box collision trigger. When the player
 *          enters the volume, plays the assigned dialogue via DialogueManager.
 *          Supports one-shot (play once then disable) or repeatable triggers.
 *
 * Owner Subsystem: Narrative
 *
 * Blueprint Extension Points:
 *   - AZP_DialogueData: set in editor details panel per-instance.
 *   - bAZP_Enabled: toggle trigger on/off at runtime.
 *   - TriggerOnce: auto-disables after first activation.
 *
 * Dependencies:
 *   - UZP_DialogueManager (on PlayerController)
 *   - UZP_DialogueData (DataAsset reference)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_DialogueTrigger.generated.h"

class UBoxComponent;
class UZP_DialogueData;

UCLASS(Blueprintable)
class THESIGNAL_API AZP_DialogueTrigger : public AActor
{
	GENERATED_BODY()

public:
	AZP_DialogueTrigger();

	/** The dialogue to play when triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TObjectPtr<UZP_DialogueData> AZP_DialogueData;

	/** If true, disables itself after first trigger. Independent of AZP_DialogueData's bAZP_OneShot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bAZP_TriggerOnce = true;

	/** Runtime enable/disable. Can be toggled by other systems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bAZP_Enabled = true;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UBoxComponent> TriggerBox;

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
};
