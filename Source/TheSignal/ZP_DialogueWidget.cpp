// Copyright The Signal. All Rights Reserved.

#include "ZP_DialogueWidget.h"
#include "ZP_DialogueManager.h"
#include "ZP_DialogueChoiceButton.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueWidget, Log, All);

void UZP_DialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Start hidden
	SetRenderOpacity(0.f);
	CurrentOpacity = 0.f;
	TargetOpacity = 0.f;

	if (ChoiceContainer)
	{
		ChoiceContainer->ClearChildren();
	}

	UE_LOG(LogDialogueWidget, Log, TEXT("[TheSignal] DialogueWidget constructed."));
}

void UZP_DialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Smooth fade
	if (!FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, 0.01f))
	{
		CurrentOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, FadeSpeed);
		SetRenderOpacity(CurrentOpacity);
	}
}

void UZP_DialogueWidget::BindToDialogueManager(UZP_DialogueManager* Manager)
{
	if (!Manager)
	{
		UE_LOG(LogDialogueWidget, Warning, TEXT("BindToDialogueManager: null manager."));
		return;
	}

	BoundManager = Manager;

	Manager->OnDialogueLineStarted.AddDynamic(this, &UZP_DialogueWidget::OnLineStarted);
	Manager->OnDialogueChoicesPresented.AddDynamic(this, &UZP_DialogueWidget::OnChoicesPresented);
	Manager->OnDialogueLineEnded.AddDynamic(this, &UZP_DialogueWidget::OnLineEnded);
	Manager->OnDialogueFinished.AddDynamic(this, &UZP_DialogueWidget::OnDialogueFinished);

	UE_LOG(LogDialogueWidget, Log, TEXT("[TheSignal] DialogueWidget bound to DialogueManager."));
}

void UZP_DialogueWidget::ShowLine(FName Speaker, const FText& Subtitle)
{
	if (SpeakerText)
	{
		SpeakerText->SetText(FText::FromName(Speaker));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(Subtitle);
	}

	ClearChoices();

	TargetOpacity = 1.f;
}

void UZP_DialogueWidget::ShowChoices(const TArray<FZP_DialogueChoice>& Choices)
{
	ClearChoices();

	if (!ChoiceContainer || !ChoiceButtonClass)
	{
		if (!ChoiceButtonClass)
		{
			UE_LOG(LogDialogueWidget, Warning, TEXT("No ChoiceButtonClass set — choices won't display. Set in WBP_DialogueBox defaults."));
		}
		return;
	}

	for (int32 i = 0; i < Choices.Num(); ++i)
	{
		UZP_DialogueChoiceButton* ButtonWidget = CreateWidget<UZP_DialogueChoiceButton>(GetOwningPlayer(), ChoiceButtonClass);
		if (!ButtonWidget) continue;

		ButtonWidget->SetupChoice(i, Choices[i].ChoiceText);
		ButtonWidget->OnChoiceClicked.AddDynamic(this, &UZP_DialogueWidget::OnChoiceClickedHandler);

		UVerticalBoxSlot* ChoiceSlot = ChoiceContainer->AddChildToVerticalBox(ButtonWidget);
		if (ChoiceSlot)
		{
			ChoiceSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
		}
	}

	// Focus first button for gamepad navigation
	if (ChoiceContainer->GetChildrenCount() > 0)
	{
		if (UZP_DialogueChoiceButton* FirstBtn = Cast<UZP_DialogueChoiceButton>(ChoiceContainer->GetChildAt(0)))
		{
			if (FirstBtn->ClickButton)
			{
				FirstBtn->ClickButton->SetKeyboardFocus();
			}
		}
	}
}

void UZP_DialogueWidget::HideDialogue()
{
	TargetOpacity = 0.f;
	ClearChoices();
}

void UZP_DialogueWidget::ClearChoices()
{
	if (ChoiceContainer)
	{
		ChoiceContainer->ClearChildren();
	}
}

// --- Delegate Handlers ---

void UZP_DialogueWidget::OnLineStarted(FName Speaker, const FText& SubtitleTextParam)
{
	ShowLine(Speaker, SubtitleTextParam);
}

void UZP_DialogueWidget::OnChoicesPresented(const TArray<FZP_DialogueChoice>& Choices)
{
	ShowChoices(Choices);
}

void UZP_DialogueWidget::OnLineEnded()
{
	ClearChoices();
}

void UZP_DialogueWidget::OnDialogueFinished()
{
	HideDialogue();
}

void UZP_DialogueWidget::OnChoiceClickedHandler(int32 ChoiceIndex)
{
	if (BoundManager)
	{
		BoundManager->SelectChoice(ChoiceIndex);
	}
}
