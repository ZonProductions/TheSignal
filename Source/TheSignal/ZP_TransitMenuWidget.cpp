// Copyright The Signal. All Rights Reserved.

#include "ZP_TransitMenuWidget.h"
#include "ZP_TransitPanel.h"
#include "ZP_TransitRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UZP_TransitButtonProxy::HandleClicked()
{
	if (!Menu.IsValid()) return;
	if (bIsClose) { Menu->CloseMenu(); }
	else { Menu->SelectEntry(DestinationIndex); }
}

static UTextBlock* MakeLabel(UWidgetTree* Tree, const FText& Text, const FLinearColor& Color, int32 FontSize = 22)
{
	UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(Text);
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), FontSize));
	return T;
}

TSharedRef<SWidget> UZP_TransitMenuWidget::RebuildWidget()
{
	// Code mode only: if the (WBP) designer didn't provide a root, build one so a bare C++ widget works.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		WidgetTree->RootWidget = RootBox;
	}
	return Super::RebuildWidget();
}

void UZP_TransitMenuWidget::InitTransitMenu(AZP_TransitPanel* InPanel, const TArray<FZP_TransitMenuEntry>& InEntries)
{
	OwningPanel = InPanel;
	Entries = InEntries;

	if (DestinationList)
	{
		BuildBoundList();   // styled WBP path
	}
	else if (RootBox)
	{
		BuildCodeList();    // pure-C++ fallback
	}
}

void UZP_TransitMenuWidget::BuildBoundList()
{
	if (!DestinationList) return;

	DestinationList->ClearChildren();
	ButtonProxies.Reset();
	RowWidgets.Reset();

	APlayerController* PC = GetOwningPlayer();

	for (const FZP_TransitMenuEntry& E : Entries)
	{
		if (RowWidgetClass && PC)
		{
			UZP_TransitRowWidget* Row = CreateWidget<UZP_TransitRowWidget>(PC, RowWidgetClass);
			if (Row)
			{
				Row->Setup(this, E);
				DestinationList->AddChild(Row);
				RowWidgets.Add(Row);
			}
		}
		else if (WidgetTree)
		{
			// Fallback: plain code button into the designer container (no row WBP assigned).
			UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Btn->SetIsEnabled(E.bAvailable);
			const FText Label = E.bAvailable ? E.DisplayName
				: FText::Format(NSLOCTEXT("Transit", "BoundLockedFmt", "{0}   [{1}]"), E.DisplayName,
					E.LockedReason.IsEmpty() ? NSLOCTEXT("Transit", "BoundLockedDef", "LOCKED") : E.LockedReason);
			Btn->AddChild(MakeLabel(WidgetTree, Label, E.bAvailable ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f)));
			if (E.bAvailable)
			{
				UZP_TransitButtonProxy* Proxy = NewObject<UZP_TransitButtonProxy>(this);
				Proxy->Menu = this;
				Proxy->DestinationIndex = E.DestinationIndex;
				Btn->OnClicked.AddDynamic(Proxy, &UZP_TransitButtonProxy::HandleClicked);
				ButtonProxies.Add(Proxy);
			}
			DestinationList->AddChild(Btn);
		}
	}

	if (CloseButton && !CloseButton->OnClicked.IsAlreadyBound(this, &UZP_TransitMenuWidget::CloseMenu))
	{
		CloseButton->OnClicked.AddDynamic(this, &UZP_TransitMenuWidget::CloseMenu);
	}
}

void UZP_TransitMenuWidget::BuildCodeList()
{
	if (!RootBox || !WidgetTree) return;

	RootBox->ClearChildren();
	ButtonProxies.Reset();

	RootBox->AddChildToVerticalBox(MakeLabel(WidgetTree,
		FText::FromString(TEXT("SELECT DESTINATION")), FLinearColor(0.9f, 0.9f, 0.9f), 26));

	for (const FZP_TransitMenuEntry& E : Entries)
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetIsEnabled(E.bAvailable);

		const FText Label = E.bAvailable ? E.DisplayName
			: FText::Format(NSLOCTEXT("Transit", "CodeLockedFmt", "{0}   [{1}]"), E.DisplayName,
				E.LockedReason.IsEmpty() ? NSLOCTEXT("Transit", "CodeLockedDef", "LOCKED") : E.LockedReason);
		Btn->AddChild(MakeLabel(WidgetTree, Label, E.bAvailable ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f)));

		if (E.bAvailable)
		{
			UZP_TransitButtonProxy* Proxy = NewObject<UZP_TransitButtonProxy>(this);
			Proxy->Menu = this;
			Proxy->DestinationIndex = E.DestinationIndex;
			Btn->OnClicked.AddDynamic(Proxy, &UZP_TransitButtonProxy::HandleClicked);
			ButtonProxies.Add(Proxy);
		}

		if (UVerticalBoxSlot* BoxSlot = RootBox->AddChildToVerticalBox(Btn))
		{
			BoxSlot->SetPadding(FMargin(0.f, 4.f));
		}
	}

	UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Close->AddChild(MakeLabel(WidgetTree, FText::FromString(TEXT("Close")), FLinearColor(0.8f, 0.8f, 0.8f)));
	UZP_TransitButtonProxy* CloseProxy = NewObject<UZP_TransitButtonProxy>(this);
	CloseProxy->Menu = this;
	CloseProxy->bIsClose = true;
	Close->OnClicked.AddDynamic(CloseProxy, &UZP_TransitButtonProxy::HandleClicked);
	ButtonProxies.Add(CloseProxy);
	if (UVerticalBoxSlot* BoxSlot = RootBox->AddChildToVerticalBox(Close))
	{
		BoxSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
}

void UZP_TransitMenuWidget::SelectEntry(int32 DestinationIndex)
{
	if (OwningPanel)
	{
		OwningPanel->TravelToDestination(DestinationIndex);
	}
	CloseMenu();
}

void UZP_TransitMenuWidget::CloseMenu()
{
	RemoveFromParent();
}
