// Copyright The Signal. All Rights Reserved.

#include "ZP_NoteSubsystem.h"

bool UZP_NoteSubsystem::AddNote(const FZP_NoteEntry& Note)
{
	if (Note.NoteID.IsNone()) return false;
	if (CollectedNoteIDs.Contains(Note.NoteID)) return false;

	CollectedNotes.Add(Note);
	CollectedNoteIDs.Add(Note.NoteID);

	UE_LOG(LogTemp, Log, TEXT("[NoteSubsystem] Added note '%s' (%s). Total: %d"),
		*Note.NoteID.ToString(), *Note.Title.ToString(), CollectedNotes.Num());

	return true;
}

bool UZP_NoteSubsystem::HasNote(FName NoteID) const
{
	return CollectedNoteIDs.Contains(NoteID);
}

void UZP_NoteSubsystem::RestoreFromSaveData(const TArray<FZP_NoteEntry>& SavedNotes)
{
	for (const FZP_NoteEntry& Note : SavedNotes)
	{
		AddNote(Note); // Deduplicates automatically
	}

	UE_LOG(LogTemp, Log, TEXT("[NoteSubsystem] Restored from save data. Total notes: %d"), CollectedNotes.Num());
}

void UZP_NoteSubsystem::ClearAll()
{
	CollectedNotes.Empty();
	CollectedNoteIDs.Empty();
	UE_LOG(LogTemp, Log, TEXT("[NoteSubsystem] Cleared all notes"));
}
