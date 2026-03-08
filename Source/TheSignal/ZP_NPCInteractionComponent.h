// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NPCInteractionComponent
 *
 * Purpose: Drop-in ActorComponent that makes any actor interactable for dialogue.
 *          Handles interaction volume, player detection, prompt, and dialogue routing.
 *          Supports both CodeSpartan DialoguePlugin (PluginDialogue + DialogueWidgetClass)
 *          and our custom system (DialogueData + ZP_DialogueManager) as fallback.
 *          Add to BP_NPC (or any actor) — zero Blueprint wiring needed.
 *
 * Owner Subsystem: Gameplay
 *
 * Blueprint Extension Points:
 *   - PluginDialogue: set to a Dialogue asset (from CodeSpartan plugin node editor).
 *   - DialogueWidgetClass: set to your WBP extending DialogueUserWidget.
 *   - DialogueData: fallback to custom ZP_DialogueManager system.
 *   - InteractionPrompt set per-instance in editor.
 *   - bInteractOnce for one-shot NPCs.
 *
 * Dependencies:
 *   - AZP_GraceCharacter (SetCurrentInteractable/ClearCurrentInteractable)
 *   - DialoguePlugin (via reflection — NO compile-time dependency)
 *   - UZP_DialogueManager (fallback, on PlayerController)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_NPCInteractionComponent.generated.h"

class UBoxComponent;
class UZP_DialogueData;
class UAnimSequenceBase;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_NPCInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_NPCInteractionComponent();

	// --- Dialogue Plugin (CodeSpartan) ---

	/** Dialogue asset from CodeSpartan's Dialogue Plugin (node editor).
	 *  Set this to use the plugin's visual dialogue system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Dialogue Plugin")
	TObjectPtr<UDataAsset> PluginDialogue;

	/** Widget class extending DialogueUserWidget (from plugin). Created on interact.
	 *  Set to your custom WBP that handles dialogue display. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Dialogue Plugin")
	TSubclassOf<UUserWidget> DialogueWidgetClass;

	// --- Custom Dialogue System (fallback) ---

	/** Our custom dialogue DataAsset. Used only if PluginDialogue is NOT set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Custom Dialogue")
	TObjectPtr<UZP_DialogueData> DialogueData;

	// --- General ---

	/** Prompt shown to the player (e.g., "Talk", "Examine"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText InteractionPrompt = FText::FromString(TEXT("Talk"));

	/** If true, interaction disables after first use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bInteractOnce = false;

	/** Name of the saved character design to load (from Character Designer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Designer", meta = (GetOptions = "GetSavedCharacterNames"))
	FName CharacterSaveName;

	// --- Dialogue Behavior ---

	/** Gesture animations to randomly play during dialogue. Assign AnimSequences in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Gestures")
	TArray<TObjectPtr<UAnimSequenceBase>> GestureAnimations;

	/** How fast the NPC rotates to face the player (degrees/sec). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Behavior")
	float FacePlayerSpeed = 5.f;

	/** Returns list of saved character names for dropdown. */
	UFUNCTION()
	TArray<FString> GetSavedCharacterNames() const;

	/** Called by the player character when they press interact while targeting this NPC. */
	void HandleInteract(class ACharacter* Interactor);

	/** Returns the prompt text for the HUD. */
	FText GetPrompt() const { return InteractionPrompt; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> InteractionVolume;

	/** Active dialogue widget (tracked so we don't double-create). */
	TWeakObjectPtr<UUserWidget> ActiveDialogueWidget;

	bool bHasBeenInteracted = false;
	bool bInDialogue = false;

	/** Cached player character for face-towards rotation. */
	TWeakObjectPtr<ACharacter> DialogueInteractor;

	/** Time until next gesture animation. */
	float GestureTimer = 0.f;

	/** Cached Sound2D property from dialogue widget (for audio-gated gestures). */
	FObjectProperty* GestureSound2DProp = nullptr;

	/** Timer that polls whether the widget has self-closed (end of dialogue). */
	FTimerHandle DialogueWatchTimer;

	/** Closes the active dialogue widget and restores game input. */
	void CloseDialogue();

	/** Called by DialogueWatchTimer to detect widget self-close. */
	void CheckDialogueWidget();

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
