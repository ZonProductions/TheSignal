// Copyright The Signal. All Rights Reserved.

#include "ZP_NotesWidget.h"
#include "ZP_NoteComponent.h"
#include "ZP_NoteEntryWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UZP_NotesWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Auto-load AZP_NoteEntryWidgetClass if BP default wasn't set
	if (!AZP_NoteEntryWidgetClass)
	{
		AZP_NoteEntryWidgetClass = LoadClass<UZP_NoteEntryWidget>(nullptr,
			TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry.WBP_NoteEntry_C"));

		UE_LOG(LogTemp, Log, TEXT("[NotesWidget] Auto-loaded AZP_NoteEntryWidgetClass: %s"),
			AZP_NoteEntryWidgetClass ? *AZP_NoteEntryWidgetClass->GetName() : TEXT("FAILED"));
	}

	// Ensure content scroll box is configured for scrolling
	if (NoteContentScroll)
	{
		NoteContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
		NoteContentScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	}
}

void UZP_NotesWidget::BindToNoteComponent(UZP_NoteComponent* InNoteComp)
{
	CachedNoteComp = InNoteComp;

	UE_LOG(LogTemp, Log, TEXT("[NotesWidget] Bound to NoteComponent: %s"),
		InNoteComp ? *InNoteComp->GetName() : TEXT("null"));
}

void UZP_NotesWidget::RefreshNoteList()
{
	UE_LOG(LogTemp, Log, TEXT("[NotesWidget] RefreshNoteList called. CachedNoteComp valid=%s, AZP_NoteEntryWidgetClass=%s, NoteListScrollBox=%s"),
		CachedNoteComp.IsValid() ? TEXT("YES") : TEXT("NO"),
		AZP_NoteEntryWidgetClass ? *AZP_NoteEntryWidgetClass->GetName() : TEXT("null"),
		NoteListScrollBox ? TEXT("valid") : TEXT("null"));

	ClearEntries();
	ClearSelection();

	if (!CachedNoteComp.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NotesWidget] CachedNoteComp is INVALID — showing empty state"));
		if (NotesEmptyText)
		{
			NotesEmptyText->SetText(AZP_NotesEmptyFallbackText);
			NotesEmptyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	const TArray<FZP_NoteEntry>& Notes = CachedNoteComp->GetNotes();
	UE_LOG(LogTemp, Log, TEXT("[NotesWidget] NoteComponent has %d notes"), Notes.Num());

	if (Notes.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[NotesWidget] Zero notes — showing empty state"));
		if (NotesEmptyText)
		{
			NotesEmptyText->SetText(AZP_NotesEmptyFallbackText);
			NotesEmptyText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	// Hide empty text
	if (NotesEmptyText)
	{
		NotesEmptyText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Create entry widgets
	if (!AZP_NoteEntryWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[NotesWidget] AZP_NoteEntryWidgetClass is null — cannot create entries"));
		return;
	}

	for (int32 i = 0; i < Notes.Num(); ++i)
	{
		UZP_NoteEntryWidget* Entry = CreateWidget<UZP_NoteEntryWidget>(this, AZP_NoteEntryWidgetClass);
		if (!Entry) continue;

		Entry->SetNoteData(Notes[i].Title, i);
		Entry->OnNoteEntryClicked.AddDynamic(this, &UZP_NotesWidget::OnEntryClicked);

		if (NoteListScrollBox)
		{
			NoteListScrollBox->AddChild(Entry);
		}

		// SetSelected after AddChild so ScrollBoxSlot exists for padding
		Entry->SetSelected(false);
		EntryWidgets.Add(Entry);
	}

	// Auto-select first note
	if (EntryWidgets.Num() > 0)
	{
		SelectNote(0);
	}
}

void UZP_NotesWidget::SelectNote(int32 Index)
{
	if (!CachedNoteComp.IsValid()) return;

	const TArray<FZP_NoteEntry>& Notes = CachedNoteComp->GetNotes();
	if (!Notes.IsValidIndex(Index)) return;

	// Update selection visuals
	for (int32 i = 0; i < EntryWidgets.Num(); ++i)
	{
		if (EntryWidgets[i])
		{
			EntryWidgets[i]->SetSelected(i == Index);
		}
	}

	SelectedIndex = Index;

	// Display note content
	const FZP_NoteEntry& Note = Notes[Index];
	if (NoteTitle)
	{
		NoteTitle->SetText(Note.Title);
	}
	if (NoteContent)
	{
		NoteContent->SetText(Note.Content);
	}

	// Scroll content back to top
	if (NoteContentScroll)
	{
		NoteContentScroll->ScrollToStart();
	}
}

void UZP_NotesWidget::ClearEntries()
{
	for (UZP_NoteEntryWidget* Entry : EntryWidgets)
	{
		if (Entry)
		{
			Entry->RemoveFromParent();
		}
	}
	EntryWidgets.Empty();
}

void UZP_NotesWidget::ClearSelection()
{
	SelectedIndex = -1;

	if (NoteTitle)
	{
		NoteTitle->SetText(FText::GetEmpty());
	}
	if (NoteContent)
	{
		NoteContent->SetText(FText::GetEmpty());
	}
}

void UZP_NotesWidget::OnEntryClicked(int32 NoteIndex)
{
	SelectNote(NoteIndex);
}
