// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * AZP_NPC
 *
 * Purpose: Base C++ actor for all Character Customizer-powered NPCs.
 *          Implements IZP_Interactable so the player can interact with NPCs.
 *          Three roles: Interactive (dialogue NPCs), Crowd (background), Corpse (dead bodies).
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - Add CharacterCustomizer component in child Blueprint (BP_NPC) for visual assembly.
 *   - Set NPCRole, DialogueData, InteractionPrompt per-instance in editor.
 *   - Override OnInteract_Implementation in Blueprint for custom behavior.
 *
 * Dependencies:
 *   - IZP_Interactable (ZP_Interactable.h)
 *   - UZP_DialogueManager (on PlayerController)
 *   - UZP_DialogueData (DataAsset)
 */

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZP_Interactable.h"
#include "ZP_NPC.generated.h"

class UBoxComponent;
class USceneComponent;
class UZP_DialogueData;

UENUM(BlueprintType)
enum class EZP_NPCNPCRole : uint8
{
	Interactive		UMETA(DisplayName = "Interactive"),
	Crowd			UMETA(DisplayName = "Crowd"),
	Corpse			UMETA(DisplayName = "Corpse")
};

UCLASS(Blueprintable)
class THESIGNAL_API AZP_NPC : public AActor, public IZP_Interactable
{
	GENERATED_BODY()

public:
	AZP_NPC();

	// --- IZP_Interactable ---
	virtual FText GetInteractionPrompt_Implementation() override;
	virtual void OnInteract_Implementation(ACharacter* Interactor) override;

	// --- Config ---

	/** NPC role determines interaction behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	EZP_NPCNPCRole NPCRole = EZP_NPCNPCRole::Interactive;

	/** Dialogue played when player interacts. Ignored for Crowd role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	TObjectPtr<UZP_DialogueData> DialogueData;

	/** Custom prompt override. Empty = auto-generated from NPCRole. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText InteractionPromptOverride;

	/** If true, interaction disables after first use (one-shot NPC). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bInteractOnce = false;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Overlap volume for player interaction detection. Disabled for Crowd role. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<UBoxComponent> InteractionVolume;

private:
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool bHasBeenInteracted = false;
};
