// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NoteComponent
 *
 * Purpose: Per-character interface to the global note collection.
 *          Delegates storage to UZP_NoteSubsystem (GameInstance lifetime)
 *          so notes persist across level transitions.
 *          Lives on Grace. Notes tab reads from this component.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - OnNoteAdded delegate for UI feedback (popup, sound).
 *   - AddNote() callable from any system (pickups, triggers, dialogue).
 *
 * Dependencies:
 *   - ZP_InventoryTabTypes.h (FZP_NoteEntry)
 *   - ZP_NoteSubsystem (GameInstanceSubsystem — canonical storage)
 */

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_NoteComponent.generated.h"

class UZP_NoteSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoteAdded, const FZP_NoteEntry&, Note);

UCLASS(ClassGroup = (TheSignal), meta = (BlueprintSpawnableComponent))
class THESIGNAL_API UZP_NoteComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZP_NoteComponent();

	/** Add a note to the permanent collection. Ignores duplicates (by NoteID). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void AddNote(const FZP_NoteEntry& Note);

	/** Returns true if a note with this ID has been collected. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	bool HasNote(FName NoteID) const;

	/** Returns the full note entry for a given ID, or nullptr if not found. */
	const FZP_NoteEntry* GetNote(FName NoteID) const;

	/** Returns all collected notes (read-only). Reads from global subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	const TArray<FZP_NoteEntry>& GetNotes() const;

	/** Returns all collected codes (notes where bIsCode is true). */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	TArray<FZP_NoteEntry> GetCodes() const;

	/** Fired when a new note is collected (only fires on this component instance). */
	UPROPERTY(BlueprintAssignable, Category = "Notes")
	FOnNoteAdded OnNoteAdded;

protected:
	virtual void BeginPlay() override;

private:
	/** Cached ref to the global note subsystem. Resolved in BeginPlay. */
	UPROPERTY()
	TObjectPtr<UZP_NoteSubsystem> NoteSubsystem;

	/** Fallback empty array for when subsystem isn't available. */
	static const TArray<FZP_NoteEntry> EmptyNotes;
};
