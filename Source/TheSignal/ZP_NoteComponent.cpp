// Copyright The Signal. All Rights Reserved.

#include "ZP_NoteComponent.h"

UZP_NoteComponent::UZP_NoteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZP_NoteComponent::AddNote(const FZP_NoteEntry& Note)
{
	if (Note.NoteID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] NoteComponent: Attempted to add note with empty NoteID — ignored."));
		return;
	}

	if (CollectedNoteIDs.Contains(Note.NoteID))
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] NoteComponent: Note '%s' already collected — skipping."), *Note.NoteID.ToString());
		return;
	}

	CollectedNotes.Add(Note);
	CollectedNoteIDs.Add(Note.NoteID);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] NoteComponent: Collected note '%s' (%s). Total: %d"),
		*Note.NoteID.ToString(), *Note.Title.ToString(), CollectedNotes.Num());

	OnNoteAdded.Broadcast(Note);
}

bool UZP_NoteComponent::HasNote(FName NoteID) const
{
	return CollectedNoteIDs.Contains(NoteID);
}

const FZP_NoteEntry* UZP_NoteComponent::GetNote(FName NoteID) const
{
	for (const FZP_NoteEntry& Note : CollectedNotes)
	{
		if (Note.NoteID == NoteID)
		{
			return &Note;
		}
	}
	return nullptr;
}

TArray<FZP_NoteEntry> UZP_NoteComponent::GetCodes() const
{
	TArray<FZP_NoteEntry> Codes;
	for (const FZP_NoteEntry& Note : CollectedNotes)
	{
		if (Note.bIsCode)
		{
			Codes.Add(Note);
		}
	}
	return Codes;
}
