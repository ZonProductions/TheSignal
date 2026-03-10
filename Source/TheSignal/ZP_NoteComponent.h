// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NoteComponent
 *
 * Purpose: Tracks notes/documents the player has collected. Lives on Grace.
 *          Notes are displayed in the Notes tab of the inventory menu.
 *          Notes with bIsCode feed into the combination safe system.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - OnNoteAdded delegate for UI feedback (popup, sound).
 *   - AddNote() callable from any system (pickups, triggers, dialogue).
 *
 * Dependencies:
 *   - ZP_InventoryTabTypes.h (FZP_NoteEntry)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_NoteComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoteAdded, const FZP_NoteEntry&, Note);

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_NoteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_NoteComponent();

	/** Add a note to the collection. Ignores duplicates (by NoteID). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void AddNote(const FZP_NoteEntry& Note);

	/** Returns true if a note with this ID has been collected. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	bool HasNote(FName NoteID) const;

	/** Returns the full note entry for a given ID, or nullptr if not found. */
	const FZP_NoteEntry* GetNote(FName NoteID) const;

	/** Returns all collected notes (read-only). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	const TArray<FZP_NoteEntry>& GetNotes() const { return CollectedNotes; }

	/** Returns all collected codes (notes where bIsCode is true). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	TArray<FZP_NoteEntry> GetCodes() const;

	/** Fired when a new note is collected. */
	UPROPERTY(BlueprintAssignable, Category = "Notes")
	FOnNoteAdded OnNoteAdded;

private:
	UPROPERTY()
	TArray<FZP_NoteEntry> CollectedNotes;

	/** Fast lookup set for deduplication. */
	TSet<FName> CollectedNoteIDs;
};
