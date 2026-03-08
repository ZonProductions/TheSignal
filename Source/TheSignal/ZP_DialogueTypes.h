// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * Dialogue Data Types
 *
 * Purpose: Structs and DataAsset for the dialogue system. A dialogue is an
 *          ordered sequence of lines, each optionally presenting player choices.
 *          Empty Choices array = linear auto-advance. Populated = show buttons.
 *
 * Owner Subsystem: Narrative
 *
 * Blueprint Extension Points:
 *   - UZP_DialogueData is a DataAsset — create instances in editor Content Browser.
 *   - All structs are BlueprintType for BP read access.
 *
 * Dependencies: None (pure data)
 */

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZP_DialogueTypes.generated.h"

/**
 * A single player choice presented during dialogue.
 * When selected, can fire a narrative beat and/or jump to another dialogue.
 */
USTRUCT(BlueprintType)
struct FZP_DialogueChoice
{
	GENERATED_BODY()

	/** Text shown on the choice button. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText ChoiceText;

	/** If set, jump to this dialogue asset after selection. NAME_None = continue current sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextDialogueID = NAME_None;

	/** Only show this choice if this beat has been triggered. NAME_None = always visible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName RequiredBeatID = NAME_None;

	/** Fire this narrative beat when chosen. NAME_None = no beat fired. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName TriggerBeatID = NAME_None;
};

/**
 * A single line of dialogue: who speaks, what they say, optional audio, optional choices.
 */
USTRUCT(BlueprintType)
struct FZP_DialogueLine
{
	GENERATED_BODY()

	/** Speaker identifier (e.g., "Ren", "Grace", "NPC_Scientist"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Speaker;

	/** Subtitle text displayed to the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText SubtitleText;

	/** Voice line audio. Null = text-only (found documents, silent moments). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USoundBase> AudioAsset = nullptr;

	/** Duration in seconds. 0 = auto-derive from audio length. If no audio, must set manually. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta=(ClampMin="0.0"))
	float Duration = 0.f;

	/** Pause after this line before advancing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta=(ClampMin="0.0"))
	float PostDelay = 0.3f;

	/** Fire this narrative beat when this line starts playing. NAME_None = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NarrativeBeatID = NAME_None;

	/** Player choices. Empty = linear (auto-advance after duration). Populated = show choice buttons. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FZP_DialogueChoice> Choices;
};

/**
 * A complete dialogue sequence — the authoring asset.
 * Create one of these per conversation, Ren monologue, NPC exchange, or found document.
 */
UCLASS(BlueprintType)
class THESIGNAL_API UZP_DialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Unique ID for save tracking and cross-dialogue jumps. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	/** Ordered lines of dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FZP_DialogueLine> Lines;

	/** If true, this dialogue plays once per save — never again after first playback. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bOneShot = true;

	/** Priority for queue management. Higher interrupts lower. Ren=100, Ambient=10. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta=(ClampMin="0"))
	int32 Priority = 50;
};
