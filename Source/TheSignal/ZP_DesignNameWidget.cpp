// Copyright The Signal. All Rights Reserved.

#include "ZP_DesignNameWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

void UZP_DesignNameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree) return;

	// Build layout programmatically — no UMG designer needed
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Root;

	UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NameRow"));
	UCanvasPanelSlot* HBoxSlot = Root->AddChildToCanvas(HBox);
	HBoxSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	HBoxSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	HBoxSlot->SetPosition(FVector2D(0.f, 20.f));
	HBoxSlot->SetAutoSize(true);

	// Label
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Label"));
	Label->SetText(FText::FromString(TEXT("Character Name: ")));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 16;
	Label->SetFont(LabelFont);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	UHorizontalBoxSlot* LabelSlot = HBox->AddChildToHorizontalBox(Label);
	LabelSlot->SetPadding(FMargin(5.f, 8.f, 5.f, 5.f));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	// Text input
	NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInput"));
	NameInput->SetHintText(FText::FromString(TEXT("Type a name...")));
	NameInput->SetMinDesiredWidth(250.f);
	NameInput->SetForegroundColor(FLinearColor::White);
	NameInput->WidgetStyle.SetForegroundColor(FSlateColor(FLinearColor::White));
	NameInput->WidgetStyle.BackgroundColor = FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f));
	UHorizontalBoxSlot* InputSlot = HBox->AddChildToHorizontalBox(NameInput);
	InputSlot->SetPadding(FMargin(5.f));
	InputSlot->SetVerticalAlignment(VAlign_Center);
	// Set Name button
	SetNameButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SetNameButton"));
	UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
	ButtonLabel->SetText(FText::FromString(TEXT("Set Name")));
	FSlateFontInfo BtnFont = ButtonLabel->GetFont();
	BtnFont.Size = 14;
	ButtonLabel->SetFont(BtnFont);
	SetNameButton->AddChild(ButtonLabel);
	UHorizontalBoxSlot* ButtonSlot = HBox->AddChildToHorizontalBox(SetNameButton);
	ButtonSlot->SetPadding(FMargin(5.f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
	SetNameButton->OnClicked.AddDynamic(this, &UZP_DesignNameWidget::OnSetNameClicked);

	// Status text
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::GetEmpty());
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 14;
	StatusText->SetFont(StatusFont);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
	UHorizontalBoxSlot* StatusSlot = HBox->AddChildToHorizontalBox(StatusText);
	StatusSlot->SetPadding(FMargin(10.f, 5.f, 5.f, 5.f));
	StatusSlot->SetVerticalAlignment(VAlign_Center);
}

void UZP_DesignNameWidget::OnSetNameClicked()
{
	if (NameInput)
	{
		ApplyName(NameInput->GetText().ToString());
	}
}

void UZP_DesignNameWidget::ApplyName(const FString& Name)
{
	if (Name.IsEmpty())
	{
		if (StatusText) StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("Name cannot be empty!")));
		return;
	}

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		if (StatusText) StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("Error: no pawn found")));
		return;
	}

	// Set DesignName via reflection (Blueprint variable on CC_Customizer_Pawn)
	UE_LOG(LogTemp, Warning, TEXT("[DesignNameWidget] Pawn class: %s"), *Pawn->GetClass()->GetName());
	FProperty* Prop = Pawn->GetClass()->FindPropertyByName(TEXT("DesignName"));
	UE_LOG(LogTemp, Warning, TEXT("[DesignNameWidget] FindPropertyByName('DesignName') = %s"), Prop ? TEXT("FOUND") : TEXT("NULL"));
	if (Prop)
	{
		FNameProperty* NameProp = CastField<FNameProperty>(Prop);
		if (NameProp)
		{
			FName NameValue(*Name);
			NameProp->SetPropertyValue_InContainer(Pawn, NameValue);

			if (StatusText) StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
			if (StatusText) StatusText->SetText(FText::FromString(FString::Printf(TEXT("Saving as: %s"), *Name)));
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] DesignName set to: %s"), *Name);
			return;
		}
	}

	if (StatusText) StatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	if (StatusText) StatusText->SetText(FText::FromString(TEXT("Error: DesignName property not found")));
}
