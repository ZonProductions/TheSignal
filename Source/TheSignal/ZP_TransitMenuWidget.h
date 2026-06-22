// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_TransitMenuWidget
 *
 * Purpose: Floor-selection menu for the Transit system. Two modes:
 *   - WBP mode: a WBP child binds a container ("DestinationList") + optional "CloseButton" and
 *     the menu spawns one UZP_TransitRowWidget (WBP_TransitRow) per destination into it. Dev styles
 *     everything in the designer.
 *   - Code mode (no WBP): the menu builds a plain vertical list of buttons in C++ (RootBox), so a
 *     placed panel works with zero assets.
 *
 * Owner Subsystem: Transit
 *
 * Dependencies: UMG, AZP_TransitPanel, UZP_TransitRowWidget.
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_TransitTypes.h"
#include "ZP_TransitMenuWidget.generated.h"

class AZP_TransitPanel;
class UVerticalBox;
class UPanelWidget;
class UButton;
class UZP_TransitMenuWidget;
class UZP_TransitRowWidget;

/** Per-button click router for the code-mode fallback (UButton::OnClicked has no params). */
UCLASS()
class UZP_TransitButtonProxy : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TWeakObjectPtr<UZP_TransitMenuWidget> Menu;

	int32 DestinationIndex = INDEX_NONE;
	bool bIsClose = false;

	UFUNCTION()
	void HandleClicked();
};

UCLASS(Blueprintable)
class THESIGNAL_API UZP_TransitMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the panel after AddToViewport: store owner + entries, build the list. */
	void InitTransitMenu(AZP_TransitPanel* InPanel, const TArray<FZP_TransitMenuEntry>& InEntries);

	/** Travel to the chosen destination (panel re-validates) and close. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void SelectEntry(int32 DestinationIndex);

	/** Remove the widget; Grace's close-watch restores game input. */
	UFUNCTION(BlueprintCallable, Category = "Transit")
	void CloseMenu();

	// --- WBP designer bindings (optional). If DestinationList is bound, WBP mode is used. ---

	/** Container the rows are added to (VerticalBox / ScrollBox named "DestinationList" in the WBP). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DestinationList;

	/** Optional close button named "CloseButton" in the WBP. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** Row widget spawned per destination (WBP_TransitRow). If unset, plain code buttons are used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transit")
	TSubclassOf<UZP_TransitRowWidget> RowWidgetClass;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	void BuildCodeList();   // pure-C++ list into RootBox (no WBP)
	void BuildBoundList();  // populate the WBP's bound DestinationList

	UPROPERTY()
	TObjectPtr<AZP_TransitPanel> OwningPanel;

	UPROPERTY()
	TArray<FZP_TransitMenuEntry> Entries;

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TArray<TObjectPtr<UZP_TransitButtonProxy>> ButtonProxies;

	UPROPERTY()
	TArray<TObjectPtr<UZP_TransitRowWidget>> RowWidgets;
};
