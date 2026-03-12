// Copyright The Signal. All Rights Reserved.

#include "ZP_NoteEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ScrollBoxSlot.h"

void UZP_NoteEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryButton)
	{
		EntryButton->OnClicked.AddDynamic(this, &UZP_NoteEntryWidget::HandleClicked);
		EntryButton->SetBackgroundColor(UnselectedBgColor);
	}
}

void UZP_NoteEntryWidget::SetNoteData(const FText& InTitle, int32 InIndex)
{
	NoteIndex = InIndex;

	if (EntryTitle)
	{
		EntryTitle->SetText(InTitle);
	}
}

void UZP_NoteEntryWidget::SetSelected(bool bSelected)
{
	if (EntryTitle)
	{
		EntryTitle->SetColorAndOpacity(FSlateColor(bSelected ? SelectedColor : UnselectedColor));
	}

	if (EntryButton)
	{
		EntryButton->SetBackgroundColor(bSelected ? SelectedBgColor : UnselectedBgColor);
	}

	// Folder-tab indent via ScrollBox slot padding
	if (UScrollBoxSlot* SBSlot = Cast<UScrollBoxSlot>(Slot))
	{
		float Left = bSelected ? SelectedLeftMargin : UnselectedLeftMargin;
		float Right = bSelected ? 0.0f : 6.0f;
		SBSlot->SetPadding(FMargin(Left, 2.0f, Right, 2.0f));
	}
}

void UZP_NoteEntryWidget::HandleClicked()
{
	OnNoteEntryClicked.Broadcast(NoteIndex);
}
