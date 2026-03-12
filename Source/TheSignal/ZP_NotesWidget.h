// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_NotesWidget
 *
 * Purpose: Two-panel notes/log viewer for the Notes tab. Scrollable note list
 *          on the left, note content on the right. Fed by UZP_NoteComponent.
 *          WBP_Notes (Blueprint) extends this — styled from EasyGameUI options menu.
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Blueprint Extension Points:
 *   - BindWidget: NoteListScrollBox (UScrollBox), NoteTitle (UTextBlock), NoteContent (UTextBlock)
 *   - BindWidgetOptional: NotesEmptyText (UTextBlock)
 *   - NoteEntryWidgetClass: set to WBP_NoteEntry in defaults.
 *
 * Dependencies: UMG, ZP_NoteComponent, ZP_NoteEntryWidget
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_NotesWidget.generated.h"

class UScrollBox;
class UTextBlock;
class UZP_NoteComponent;
class UZP_NoteEntryWidget;

UCLASS()
class THESIGNAL_API UZP_NotesWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	/** Scrollable list of note entries (left panel). */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> NoteListScrollBox;

	/** Title of the currently selected note (right panel header). */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NoteTitle;

	/** Body content of the currently selected note (right panel body). */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NoteContent;

	/** ScrollBox wrapping NoteContent for scrollable long notes. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UScrollBox> NoteContentScroll;

	/** Shown when no notes are collected. Hidden when list has entries. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> NotesEmptyText;

	/** Widget class for individual note entries. Set to WBP_NoteEntry. */
	UPROPERTY(EditDefaultsOnly, Category = "Notes")
	TSubclassOf<UZP_NoteEntryWidget> NoteEntryWidgetClass;

	/** Bind to the player's NoteComponent. Call once after creation. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void BindToNoteComponent(UZP_NoteComponent* InNoteComp);

	/** Rebuild the note list from the NoteComponent data. Auto-selects first entry. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void RefreshNoteList();

	/** Select and display a specific note by index. */
	UFUNCTION(BlueprintCallable, Category = "Notes")
	void SelectNote(int32 Index);

private:
	UPROPERTY()
	TWeakObjectPtr<UZP_NoteComponent> CachedNoteComp;

	UPROPERTY()
	TArray<TObjectPtr<UZP_NoteEntryWidget>> EntryWidgets;

	int32 SelectedIndex = -1;

	void ClearEntries();
	void ClearSelection();

	UFUNCTION()
	void OnEntryClicked(int32 NoteIndex);
};
