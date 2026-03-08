// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_DialogueWidget
 *
 * Purpose: C++ base class for the dialogue UI widget. Handles subtitle display,
 *          speaker name, and choice button management. WBP_DialogueBox (Blueprint)
 *          extends this for visual layout using EGUI-styled widgets.
 *
 * Owner Subsystem: Narrative
 *
 * Blueprint Extension Points:
 *   - BindWidget properties map to UMG designer widget names.
 *   - ChoiceButtonClass: set to WBP_DialogueChoiceButton in Blueprint defaults.
 *   - All Show/Hide functions are BlueprintCallable.
 *
 * Dependencies:
 *   - UMG (UUserWidget, UTextBlock, UVerticalBox)
 *   - UZP_DialogueManager (binds to its delegates)
 *   - UZP_DialogueChoiceButton (spawned for each choice)
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_DialogueTypes.h"
#include "ZP_DialogueWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UImage;
class UZP_DialogueManager;
class UZP_DialogueChoiceButton;

UCLASS()
class THESIGNAL_API UZP_DialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- BindWidget (names MUST match UMG designer widget names) ---

	/** Speaker name display. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SpeakerText;

	/** Subtitle/dialogue line display. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SubtitleText;

	/** Container for dynamically spawned choice buttons. */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> ChoiceContainer;

	/** Background panel — fades in/out with dialogue. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UImage> DialogueBackground;

	// --- Config ---

	/** Widget class for choice buttons. Must be UZP_DialogueChoiceButton subclass.
	 *  Set to WBP_DialogueChoiceButton in Blueprint defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<UZP_DialogueChoiceButton> ChoiceButtonClass;

	/** How fast the widget fades in/out. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	float FadeSpeed = 8.f;

	// --- API ---

	/** Bind this widget to a DialogueManager's delegates. Call once after creation. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BindToDialogueManager(UZP_DialogueManager* Manager);

	/** Show a dialogue line (speaker + subtitle). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ShowLine(FName Speaker, const FText& Subtitle);

	/** Show choice buttons. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void ShowChoices(const TArray<FZP_DialogueChoice>& Choices);

	/** Hide everything (dialogue finished). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void HideDialogue();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UZP_DialogueManager> BoundManager;

	float TargetOpacity = 0.f;
	float CurrentOpacity = 0.f;

	void ClearChoices();

	// Delegate handlers
	UFUNCTION()
	void OnLineStarted(FName Speaker, const FText& SubtitleTextParam);

	UFUNCTION()
	void OnChoicesPresented(const TArray<FZP_DialogueChoice>& Choices);

	UFUNCTION()
	void OnLineEnded();

	UFUNCTION()
	void OnDialogueFinished();

	UFUNCTION()
	void OnChoiceClickedHandler(int32 ChoiceIndex);
};
