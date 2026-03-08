// Copyright The Signal. All Rights Reserved.

#include "ZP_DialogueChoiceButton.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UZP_DialogueChoiceButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (ClickButton)
	{
		ClickButton->OnClicked.AddDynamic(this, &UZP_DialogueChoiceButton::HandleClicked);
	}
}

void UZP_DialogueChoiceButton::SetupChoice(int32 InIndex, const FText& InText)
{
	ChoiceIndex = InIndex;

	if (ButtonText)
	{
		ButtonText->SetText(InText);
	}
}

void UZP_DialogueChoiceButton::HandleClicked()
{
	OnChoiceClicked.Broadcast(ChoiceIndex);
}
