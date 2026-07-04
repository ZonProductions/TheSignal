// Copyright The Signal. All Rights Reserved.

#include "ZP_DialogueManager.h"
#include "ZP_EventBroadcaster.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogue, Log, All);

UZP_DialogueManager::UZP_DialogueManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UZP_DialogueManager::BeginPlay()
{
	Super::BeginPlay();

	// Cache event broadcaster
	if (UGameInstance* GI = GetOwner()->GetGameInstance())
	{
		EventBroadcaster = GI->GetSubsystem<UZP_EventBroadcaster>();
	}

	// Create a persistent audio component for voice playback
	VoiceAudioComponent = NewObject<UAudioComponent>(GetOwner(), TEXT("DialogueVoiceAudio"));
	if (VoiceAudioComponent)
	{
		VoiceAudioComponent->RegisterComponent();
		VoiceAudioComponent->bAutoActivate = false;
		VoiceAudioComponent->bAutoDestroy = false;
		// 2D playback — dialogue is non-spatial (earpiece, subtitles)
		VoiceAudioComponent->bIsUISound = true;
	}

	// Auto-discover all ZP_DialogueData assets and register them
	{
		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AR = ARM.Get();

		TArray<FAssetData> DialogueAssets;
		AR.GetAssetsByClass(UZP_DialogueData::StaticClass()->GetClassPathName(), DialogueAssets, true);

		for (const FAssetData& AssetData : DialogueAssets)
		{
			if (UZP_DialogueData* DA = Cast<UZP_DialogueData>(AssetData.GetAsset()))
			{
				RegisterDialogue(DA);
			}
		}

		UE_LOG(LogDialogue, Log, TEXT("[TheSignal] DialogueManager: auto-registered %d dialogue assets."), AZP_DialogueLookupTable.Num());
	}

	UE_LOG(LogDialogue, Log, TEXT("[TheSignal] DialogueManager initialized."));
}

void UZP_DialogueManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsPlaying || !ActiveDialogue) return;

	// Waiting for player to pick a choice — don't tick timer
	if (bWaitingForChoice) return;

	// Post-delay between lines
	if (bWaitingForPostDelay)
	{
		PostDelayTimer -= DeltaTime;
		if (PostDelayTimer <= 0.f)
		{
			bWaitingForPostDelay = false;
			AdvanceToNextLine();
		}
		return;
	}

	// Tick line duration
	LineTimer -= DeltaTime;
	if (LineTimer <= 0.f)
	{
		OnDialogueLineEnded.Broadcast();

		const FZP_DialogueLine& Line = ActiveDialogue->AZP_Lines[CurrentLineIndex];
		if (Line.PostDelay > 0.f)
		{
			bWaitingForPostDelay = true;
			PostDelayTimer = Line.PostDelay;
		}
		else
		{
			AdvanceToNextLine();
		}
	}
}

// --- API ---

void UZP_DialogueManager::RegisterDialogue(UZP_DialogueData* Dialogue)
{
	if (!Dialogue || Dialogue->AZP_DialogueID == NAME_None) return;

	if (!AZP_DialogueLookupTable.Contains(Dialogue->AZP_DialogueID))
	{
		AZP_DialogueLookupTable.Add(Dialogue->AZP_DialogueID, Dialogue);
		UE_LOG(LogDialogue, Log, TEXT("Registered dialogue '%s'."), *Dialogue->AZP_DialogueID.ToString());
	}

	// Also register any DAs referenced by choice jumps (recursive discovery)
	for (const FZP_DialogueLine& Line : Dialogue->AZP_Lines)
	{
		for (const FZP_DialogueChoice& Choice : Line.Choices)
		{
			if (Choice.NextDialogueID != NAME_None && !AZP_DialogueLookupTable.Contains(Choice.NextDialogueID))
			{
				// Try to find the DA by scanning loaded assets with matching DialogueID
				// For now, the NPC interaction component should register all its response DAs
				UE_LOG(LogDialogue, Verbose, TEXT("  Choice references unregistered dialogue '%s'."), *Choice.NextDialogueID.ToString());
			}
		}
	}
}

void UZP_DialogueManager::PlayDialogue(UZP_DialogueData* Dialogue)
{
	if (!Dialogue || Dialogue->AZP_Lines.Num() == 0)
	{
		UE_LOG(LogDialogue, Warning, TEXT("PlayDialogue called with null or empty dialogue."));
		return;
	}

	// Auto-register this DA for future ID-based lookups
	RegisterDialogue(Dialogue);

	// One-shot check
	if (Dialogue->bAZP_OneShot && PlayedDialogueIDs.Contains(Dialogue->AZP_DialogueID))
	{
		UE_LOG(LogDialogue, Log, TEXT("Skipping one-shot dialogue '%s' — already played."), *Dialogue->AZP_DialogueID.ToString());
		return;
	}

	// If something is playing, compare priority
	if (bIsPlaying && ActiveDialogue)
	{
		if (Dialogue->AZP_Priority > ActiveDialogue->AZP_Priority)
		{
			// Interrupt current, queue the interrupted one (if not one-shot-already-counted)
			UE_LOG(LogDialogue, Log, TEXT("Dialogue '%s' (pri %d) interrupting '%s' (pri %d)."),
				*Dialogue->AZP_DialogueID.ToString(), Dialogue->AZP_Priority,
				*ActiveDialogue->AZP_DialogueID.ToString(), ActiveDialogue->AZP_Priority);
			StopDialogue();
		}
		else
		{
			// Queue the new one
			DialogueQueue.Add({ Dialogue, Dialogue->AZP_Priority });
			// Sort queue by priority (highest first)
			DialogueQueue.Sort([](const FQueuedDialogue& A, const FQueuedDialogue& B)
			{
				return A.Priority > B.Priority;
			});
			UE_LOG(LogDialogue, Log, TEXT("Queued dialogue '%s' (pri %d). Queue size: %d."),
				*Dialogue->AZP_DialogueID.ToString(), Dialogue->AZP_Priority, DialogueQueue.Num());
			return;
		}
	}

	// Start playback
	ActiveDialogue = Dialogue;
	bIsPlaying = true;
	bWaitingForChoice = false;
	bWaitingForPostDelay = false;
	SetComponentTickEnabled(true);

	OnDialogueStarted.Broadcast(Dialogue);
	if (EventBroadcaster)
	{
		EventBroadcaster->BroadcastDialogueStarted(Dialogue->AZP_DialogueID);
	}
	UE_LOG(LogDialogue, Log, TEXT("Starting dialogue '%s' (%d lines)."),
		*Dialogue->AZP_DialogueID.ToString(), Dialogue->AZP_Lines.Num());

	StartLine(0);
}

void UZP_DialogueManager::PlayDialogueByID(FName DialogueID)
{
	if (TObjectPtr<UZP_DialogueData>* Found = AZP_DialogueLookupTable.Find(DialogueID))
	{
		PlayDialogue(*Found);
	}
	else
	{
		UE_LOG(LogDialogue, Warning, TEXT("PlayDialogueByID: '%s' not found in lookup table."), *DialogueID.ToString());
	}
}

void UZP_DialogueManager::StopDialogue()
{
	if (!bIsPlaying) return;

	if (VoiceAudioComponent && VoiceAudioComponent->IsPlaying())
	{
		VoiceAudioComponent->Stop();
	}

	OnDialogueLineEnded.Broadcast();
	FinishDialogue();
}

void UZP_DialogueManager::SelectChoice(int32 ChoiceIndex)
{
	if (!bWaitingForChoice || !ActiveDialogue) return;

	const FZP_DialogueLine& Line = ActiveDialogue->AZP_Lines[CurrentLineIndex];
	TArray<FZP_DialogueChoice> FilteredChoices = FilterChoices(Line.Choices);

	if (!FilteredChoices.IsValidIndex(ChoiceIndex))
	{
		UE_LOG(LogDialogue, Warning, TEXT("SelectChoice: invalid index %d (max %d)."), ChoiceIndex, FilteredChoices.Num() - 1);
		return;
	}

	const FZP_DialogueChoice& Choice = FilteredChoices[ChoiceIndex];

	UE_LOG(LogDialogue, Log, TEXT("Player chose [%d]: '%s'"), ChoiceIndex, *Choice.ChoiceText.ToString());

	// Fire narrative beat if specified
	if (Choice.TriggerBeatID != NAME_None)
	{
		TriggeredBeats.Add(Choice.TriggerBeatID);
		if (EventBroadcaster)
		{
			EventBroadcaster->BroadcastNarrativeBeat(Choice.TriggerBeatID);
		}
	}

	bWaitingForChoice = false;
	OnDialogueLineEnded.Broadcast();

	// Jump to another dialogue or continue
	if (Choice.NextDialogueID != NAME_None)
	{
		FinishDialogue();
		PlayDialogueByID(Choice.NextDialogueID);
	}
	else
	{
		AdvanceToNextLine();
	}
}

bool UZP_DialogueManager::HasDialoguePlayed(FName DialogueID) const
{
	return PlayedDialogueIDs.Contains(DialogueID);
}

// --- Internal ---

void UZP_DialogueManager::StartLine(int32 Index)
{
	if (!ActiveDialogue || !ActiveDialogue->AZP_Lines.IsValidIndex(Index))
	{
		FinishDialogue();
		return;
	}

	CurrentLineIndex = Index;
	const FZP_DialogueLine& Line = ActiveDialogue->AZP_Lines[Index];

	UE_LOG(LogDialogue, Log, TEXT("  Line %d — %s: \"%s\""),
		Index, *Line.Speaker.ToString(), *Line.SubtitleText.ToString());

	// Fire narrative beat
	if (Line.NarrativeBeatID != NAME_None)
	{
		TriggeredBeats.Add(Line.NarrativeBeatID);
		if (EventBroadcaster)
		{
			EventBroadcaster->BroadcastNarrativeBeat(Line.NarrativeBeatID);
		}
	}

	// Play audio
	if (Line.AudioAsset && VoiceAudioComponent)
	{
		VoiceAudioComponent->SetSound(Line.AudioAsset);
		VoiceAudioComponent->Play();
	}

	// Broadcast to UI
	OnDialogueLineStarted.Broadcast(Line.Speaker, Line.SubtitleText);

	// Check for choices
	TArray<FZP_DialogueChoice> FilteredChoices = FilterChoices(Line.Choices);
	if (FilteredChoices.Num() > 0)
	{
		// Show choices, pause for player input
		bWaitingForChoice = true;
		LineTimer = 0.f; // Timer irrelevant while waiting for choice
		OnDialogueChoicesPresented.Broadcast(FilteredChoices);
	}
	else
	{
		// Linear — auto-advance after duration
		bWaitingForChoice = false;
		LineTimer = GetLineDuration(Line);
	}
}

void UZP_DialogueManager::AdvanceToNextLine()
{
	if (!ActiveDialogue) return;

	int32 NextIndex = CurrentLineIndex + 1;
	if (NextIndex < ActiveDialogue->AZP_Lines.Num())
	{
		StartLine(NextIndex);
	}
	else
	{
		FinishDialogue();
	}
}

void UZP_DialogueManager::FinishDialogue()
{
	if (ActiveDialogue)
	{
		// Mark as played for one-shot tracking
		if (ActiveDialogue->bAZP_OneShot && ActiveDialogue->AZP_DialogueID != NAME_None)
		{
			PlayedDialogueIDs.Add(ActiveDialogue->AZP_DialogueID);
		}
		if (EventBroadcaster)
		{
			EventBroadcaster->BroadcastDialogueFinished(ActiveDialogue->AZP_DialogueID);
		}
		UE_LOG(LogDialogue, Log, TEXT("Dialogue '%s' finished."), *ActiveDialogue->AZP_DialogueID.ToString());
	}

	ActiveDialogue = nullptr;
	CurrentLineIndex = -1;
	LineTimer = 0.f;
	bIsPlaying = false;
	bWaitingForChoice = false;
	bWaitingForPostDelay = false;
	SetComponentTickEnabled(false);

	OnDialogueFinished.Broadcast();

	// Play next queued dialogue if any
	PlayNextFromQueue();
}

float UZP_DialogueManager::GetLineDuration(const FZP_DialogueLine& Line) const
{
	// Explicit duration takes precedence
	if (Line.Duration > 0.f) return Line.Duration;

	// Auto-derive from audio
	if (Line.AudioAsset)
	{
		return Line.AudioAsset->GetDuration();
	}

	// Text-only fallback: ~50ms per character, minimum 2 seconds
	float TextDuration = FMath::Max(AZP_MinSubtitleDuration, Line.SubtitleText.ToString().Len() * AZP_SubtitleSecondsPerChar);
	return TextDuration;
}

void UZP_DialogueManager::PlayNextFromQueue()
{
	if (DialogueQueue.Num() == 0) return;

	// Queue is already sorted by priority (highest first)
	FQueuedDialogue Next = DialogueQueue[0];
	DialogueQueue.RemoveAt(0);

	UE_LOG(LogDialogue, Log, TEXT("Playing next from queue: '%s' (pri %d). Remaining: %d."),
		*Next.Dialogue->AZP_DialogueID.ToString(), Next.Priority, DialogueQueue.Num());

	PlayDialogue(Next.Dialogue);
}

TArray<FZP_DialogueChoice> UZP_DialogueManager::FilterChoices(const TArray<FZP_DialogueChoice>& Choices) const
{
	TArray<FZP_DialogueChoice> Result;
	for (const FZP_DialogueChoice& Choice : Choices)
	{
		// If no required beat, always visible
		if (Choice.RequiredBeatID == NAME_None)
		{
			Result.Add(Choice);
			continue;
		}

		// Check if required beat has been triggered
		if (TriggeredBeats.Contains(Choice.RequiredBeatID))
		{
			Result.Add(Choice);
		}
	}
	return Result;
}
