// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_DesignNameWidget
 *
 * Purpose: Overlay widget for the Character Designer. Provides a text input
 *          field and "Set Name" button. Sets DesignName on the CC_Customizer_Pawn
 *          so the CC save system stores the character under a custom name.
 *
 * Owner Subsystem: NPC Pipeline
 *
 * Usage: Created by CC_GameMode after spawning the customizer pawn.
 *        Appears at top of screen. User types name, clicks Set Name (or Enter).
 *        Then uses CC's normal save/exit to persist.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_DesignNameWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;
class UHorizontalBox;

UCLASS()
class THESIGNAL_API UZP_DesignNameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY()
	TObjectPtr<UEditableTextBox> NameInput;

	UPROPERTY()
	TObjectPtr<UButton> SetNameButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UFUNCTION()
	void OnSetNameClicked();

	void ApplyName(const FString& Name);
};
