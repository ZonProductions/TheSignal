// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitRowWidget.h"
#include "ZP_TransitMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UZP_TransitRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RowButton)
	{
		if (!RowButton->OnClicked.IsAlreadyBound(this, &UZP_TransitRowWidget::HandleClicked))
		{
			RowButton->OnClicked.AddDynamic(this, &UZP_TransitRowWidget::HandleClicked);
		}
	}
	else
	{
		// No button: make the whole row hit-testable so NativeOnMouseButtonDown fires.
		SetVisibility(ESlateVisibility::Visible);
	}
}

UTextBlock* UZP_TransitRowWidget::ResolveLabel() const
{
	if (RowLabel)
	{
		return RowLabel;
	}
	UTextBlock* Found = nullptr;
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([&Found](UWidget* W)
		{
			if (!Found)
			{
				if (UTextBlock* T = Cast<UTextBlock>(W))
				{
					Found = T;
				}
			}
		});
	}
	return Found;
}

void UZP_TransitRowWidget::Setup(UZP_TransitMenuWidget* InMenu, const FZP_TransitMenuEntry& Entry)
{
	Menu = InMenu;
	DestinationIndex = Entry.DestinationIndex;
	bAvailable = Entry.bAvailable;

	if (UTextBlock* Label = ResolveLabel())
	{
		const FText Text = Entry.bAvailable
			? Entry.DisplayName
			: FText::Format(NSLOCTEXT("Transit", "RowLockedFmt", "{0}   [{1}]"),
				Entry.DisplayName,
				Entry.LockedReason.IsEmpty() ? NSLOCTEXT("Transit", "RowLockedDef", "LOCKED") : Entry.LockedReason);
		Label->SetText(Text);
	}

	if (RowButton)
	{
		RowButton->SetIsEnabled(Entry.bAvailable);
	}
}

FReply UZP_TransitRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	HandleClicked();
	return FReply::Handled();
}

void UZP_TransitRowWidget::HandleClicked()
{
	if (Menu && bAvailable)
	{
		Menu->SelectEntry(DestinationIndex);
	}
}
