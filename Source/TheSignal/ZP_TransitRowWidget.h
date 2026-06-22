// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_TransitRowWidget
 *
 * Purpose: One row in the transit floor list. Flexible by design so the dev can lay the row out
 *          any way in WBP_TransitRow:
 *            - Label: binds "RowLabel" if present, else auto-uses the first TextBlock in the row.
 *            - Click: binds "RowButton" OnClicked if present, else the whole row is clickable.
 *          The menu spawns one row per destination and calls Setup().
 *
 * Owner Subsystem: Transit
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_TransitTypes.h"
#include "ZP_TransitRowWidget.generated.h"

class UButton;
class UTextBlock;
class UZP_TransitMenuWidget;

UCLASS(Blueprintable)
class THESIGNAL_API UZP_TransitRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Optional: a button to drive the click. If absent, the whole row is clickable. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RowButton;

	/** Optional: the label. If absent, the first TextBlock in the row is used. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RowLabel;

	/** Populate this row from a menu entry. */
	void Setup(UZP_TransitMenuWidget* InMenu, const FZP_TransitMenuEntry& Entry);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleClicked();

	/** RowLabel if bound, else the first TextBlock found in the widget tree. */
	UTextBlock* ResolveLabel() const;

private:
	UPROPERTY()
	TObjectPtr<UZP_TransitMenuWidget> Menu;

	int32 DestinationIndex = INDEX_NONE;
	bool bAvailable = true;
};
