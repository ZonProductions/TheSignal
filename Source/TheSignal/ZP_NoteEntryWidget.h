// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NoteEntryWidget
 *
 * Purpose: Single note entry in the Notes tab list. Mirrors ZP_DialogueChoiceButton
 *          pattern — click routes back to parent NotesWidget via delegate.
 *          WBP_NoteEntry (Blueprint) extends this for EGUI styling.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - BindWidget: EntryButton (UButton), EntryTitle (UTextBlock)
 *   - Style the WBP using EGUI CommonBackground visuals.
 *
 * Dependencies: UMG
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_NoteEntryWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNoteEntryClicked, int32, NoteIndex);

UCLASS()
class THESIGNAL_API UZP_NoteEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The clickable button. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> EntryButton;

	/** Title text displayed on the entry. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> EntryTitle;

	/** Set the note index and display title. Call after creation. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void SetNoteData(const FText& InTitle, int32 InIndex);

	/** Update visual state for selected/unselected. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void SetSelected(bool bSelected);

	/** Fires when this entry is clicked, carrying the note index. */
	UPROPERTY(BlueprintAssignable, Category = "Notes")
	FOnNoteEntryClicked OnNoteEntryClicked;

	/** Color for the selected entry title. */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	FLinearColor SelectedColor = FLinearColor(0.92f, 0.92f, 0.92f, 1.0f);

	/** Color for unselected entry titles. */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	FLinearColor UnselectedColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.9f);

	/** Button background when selected (teal accent). */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	FLinearColor SelectedBgColor = FLinearColor(0.05f, 0.16f, 0.12f, 1.0f);

	/** Button background when unselected. */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	FLinearColor UnselectedBgColor = FLinearColor(0.04f, 0.04f, 0.04f, 0.6f);

	/** Left margin when selected (folder-tab indent). */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	float SelectedLeftMargin = 14.0f;

	/** Left margin when unselected. */
	UPROPERTY(EditDefaultsOnly, Category = "Notes|Style")
	float UnselectedLeftMargin = 0.0f;

protected:
	virtual void NativeConstruct() override;

private:
	int32 NoteIndex = -1;

	UFUNCTION()
	void HandleClicked();
};
