// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_DialogueManager
 *
 * Purpose: ActorComponent on PlayerController that orchestrates dialogue playback.
 *          Ticks through dialogue lines, plays audio, fires subtitle/choice events
 *          to the UI widget, manages queue and priority, tracks one-shot playback
 *          for save persistence.
 *
 * Owner Subsystem: Narrative
 *
 * Blueprint Extension Points:
 *   - All delegates are BlueprintAssignable for widget binding.
 *   - PlayDialogue / StopDialogue / SelectChoice are BlueprintCallable.
 *   - DialogueLookupTable maps FName DialogueIDs to DataAssets (set in Blueprint).
 *
 * Dependencies:
 *   - UZP_DialogueData, FZP_DialogueLine, FZP_DialogueChoice (ZP_DialogueTypes.h)
 *   - UZP_EventBroadcaster (for narrative beat firing)
 *   - UAudioComponent (for voice playback)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_DialogueTypes.h"
#include "ZP_DialogueManager.generated.h"

class UAudioComponent;
class UZP_EventBroadcaster;

// --- Delegate Signatures ---

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineStarted, FName, Speaker, const FText&, SubtitleText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueChoicesPresented, const TArray<FZP_DialogueChoice>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueLineEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStarted, UZP_DialogueData*, Dialogue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueFinished);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THESIGNAL_API UZP_DialogueManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_DialogueManager();

	// --- API ---

	/** Start playing a dialogue sequence. Queues if something higher-priority is active. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void PlayDialogue(UZP_DialogueData* Dialogue);

	/** Play a dialogue by ID lookup. Requires DialogueLookupTable to be populated. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void PlayDialogueByID(FName DialogueID);

	/** Interrupt and stop current dialogue. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StopDialogue();

	/** Player selected a choice. Called by the UI widget. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SelectChoice(int32 ChoiceIndex);

	/** Is a dialogue currently playing? */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialoguePlaying() const { return bIsPlaying; }

	/** Has a one-shot dialogue already been played? */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool HasDialoguePlayed(FName DialogueID) const;

	// --- Save Data ---

	/** Get all played dialogue IDs (for save serialization). */
	UFUNCTION(BlueprintPure, Category = "Dialogue|Save")
	TSet<FName> GetPlayedDialogueIDs() const { return PlayedDialogueIDs; }

	/** Restore played dialogue IDs (from save deserialization). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Save")
	void SetPlayedDialogueIDs(const TSet<FName>& InIDs) { PlayedDialogueIDs = InIDs; }

	/** Get all triggered narrative beats (for save serialization). */
	UFUNCTION(BlueprintPure, Category = "Dialogue|Save")
	TSet<FName> GetTriggeredBeats() const { return TriggeredBeats; }

	/** Restore triggered beats (from save deserialization). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Save")
	void SetTriggeredBeats(const TSet<FName>& InBeats) { TriggeredBeats = InBeats; }

	// --- Delegates ---

	/** Fired when a new line starts. Widget shows speaker + subtitle. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueLineStarted OnDialogueLineStarted;

	/** Fired when a line has choices. Widget shows choice buttons. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueChoicesPresented OnDialogueChoicesPresented;

	/** Fired when a line ends (before next line or choice). Widget clears text. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueLineEnded OnDialogueLineEnded;

	/** Fired when a dialogue sequence begins. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueStarted OnDialogueStarted;

	/** Fired when a dialogue sequence finishes (all lines played or stopped). */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueFinished OnDialogueFinished;

	// --- Config ---

	/** Map of DialogueID → DialogueData for ID-based lookups and choice jumps.
	 *  Auto-populated: RegisterDialogue() called when NPC interaction components register,
	 *  and PlayDialogue auto-registers any DA it plays. Can also be pre-populated in editor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TMap<FName, TObjectPtr<UZP_DialogueData>> DialogueLookupTable;

	/** Register a dialogue DA for ID-based lookup. Called automatically by NPCInteractionComponent. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void RegisterDialogue(UZP_DialogueData* Dialogue);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// --- Playback State ---
	UPROPERTY()
	TObjectPtr<UZP_DialogueData> ActiveDialogue;

	int32 CurrentLineIndex = -1;
	float LineTimer = 0.f;
	bool bIsPlaying = false;
	bool bWaitingForChoice = false;
	bool bWaitingForPostDelay = false;
	float PostDelayTimer = 0.f;

	// --- Audio ---
	UPROPERTY()
	TObjectPtr<UAudioComponent> VoiceAudioComponent;

	// --- Queue ---
	struct FQueuedDialogue
	{
		TObjectPtr<UZP_DialogueData> Dialogue;
		int32 Priority;
	};
	TArray<FQueuedDialogue> DialogueQueue;

	// --- Save Tracking ---
	TSet<FName> PlayedDialogueIDs;
	TSet<FName> TriggeredBeats;

	// --- Cached ---
	UPROPERTY()
	TObjectPtr<UZP_EventBroadcaster> EventBroadcaster;

	// --- Internal ---
	void StartLine(int32 Index);
	void AdvanceToNextLine();
	void FinishDialogue();
	float GetLineDuration(const FZP_DialogueLine& Line) const;
	void PlayNextFromQueue();
	TArray<FZP_DialogueChoice> FilterChoices(const TArray<FZP_DialogueChoice>& Choices) const;
};
