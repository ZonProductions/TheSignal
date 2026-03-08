// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_DialogueChoiceButton
 *
 * Purpose: Small C++ base for dialogue choice buttons. Knows its index and
 *          routes click events back to the DialogueWidget. WBP_DialogueChoiceButton
 *          (Blueprint) extends this for EGUI styling.
 *
 * Owner Subsystem: Narrative
 *
 * Blueprint Extension Points:
 *   - BindWidget: ButtonText (UTextBlock), ClickButton (UButton)
 *   - Style the WBP using EGUI CommonButton visuals.
 *
 * Dependencies: UMG
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_DialogueChoiceButton.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChoiceButtonClicked, int32, ChoiceIndex);

UCLASS()
class THESIGNAL_API UZP_DialogueChoiceButton : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Text displayed on the button. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ButtonText;

	/** The clickable button. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ClickButton;

	/** Set the choice index and display text. Call after creation. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetupChoice(int32 InIndex, const FText& InText);

	/** Fires when this button is clicked, carrying the choice index. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnChoiceButtonClicked OnChoiceClicked;

protected:
	virtual void NativeConstruct() override;

private:
	int32 ChoiceIndex = -1;

	UFUNCTION()
	void HandleClicked();
};
