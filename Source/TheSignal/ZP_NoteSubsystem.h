// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NoteSubsystem
 *
 * Purpose: Canonical storage for collected notes/documents. Persists across
 *          level transitions (GameInstance lifetime). NoteComponent on
 *          GraceCharacter syncs from this subsystem each BeginPlay.
 *          Notes are permanent once collected — they never leave.
 *
 * Owner Subsystem: SaveSystem
 *
 * Dependencies: ZP_InventoryTabTypes (FZP_NoteEntry)
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_NoteSubsystem.generated.h"

UCLASS()
class THESIGNAL_API UZP_NoteSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Add a note. Ignores duplicates by NoteID. Returns true if actually added. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	bool AddNote(const FZP_NoteEntry& Note);

	/** Returns true if a note with this ID has been collected. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	bool HasNote(FName NoteID) const;

	/** Returns all collected notes (read-only). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	const TArray<FZP_NoteEntry>& GetNotes() const { return CollectedNotes; }

	/** Returns note count. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	int32 GetNoteCount() const { return CollectedNotes.Num(); }

	/** Restore notes from save data. Merges (deduplicates) with existing. */
	void RestoreFromSaveData(const TArray<FZP_NoteEntry>& SavedNotes);

	/** Clear all notes (call on New Game). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void ClearAll();

private:
	UPROPERTY()
	TArray<FZP_NoteEntry> CollectedNotes;

	TSet<FName> CollectedNoteIDs;
};
