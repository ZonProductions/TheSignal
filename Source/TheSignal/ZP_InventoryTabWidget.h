// Copyright The Signal. All Rights Reserved.

#pragma once

/**
 * UZP_InventoryTabWidget
 *
 * Purpose: Tab controller for unified inventory menu (Map, Inventory, Notes).
 *          Does NOT create or embed the Moonville widget — instead, after
 *          Moonville's ToggleInventoryMenu opens WBP_InventoryMenu_Horror,
 *          this widget finds it, injects tab buttons into the user-placed
 *          "TabHeader", and manages tab switching (hide/show inventory
 *          content vs map/notes content).
 *
 * Owner Subsystem: PlayerCharacter
 *
 * Dependencies:
 *   - UMG (UUserWidget, UButton, UImage, UTextBlock)
 *   - UZP_MapComponent (map area data)
 *   - UZP_NoteComponent (collected notes)
 *   - AZP_MapVolume (world bounds for coordinate conversion)
 *   - WBP_InventoryMenu_Horror (Moonville inventory widget — found, not created)
 */

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_InventoryTabWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UCanvasPanel;
class UPanelWidget;
class UHorizontalBox;
class UOverlaySlot;
class AZP_GraceCharacter;
class UZP_MapComponent;
class UZP_NoteComponent;
class AZP_MapVolume;
class UZP_NotesWidget;

UCLASS()
class THESIGNAL_API UZP_InventoryTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Config ---

	/** Size of the player marker on the map (pixels). */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab|Map")
	FVector2D TabMarkerSize = FVector2D(20.0f, 20.0f);

	/** Color for the active tab button text. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab|Style")
	FLinearColor ActiveTabColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

	/** Color for inactive tab button text. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab|Style")
	FLinearColor InactiveTabColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.6f);

	/** Widget class for Moonville inventory (used to find it in viewport). */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	/** Widget class for the Notes panel (WBP_Notes). Auto-loaded if not set. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab")
	TSubclassOf<UZP_NotesWidget> NotesWidgetClass;

	// --- API ---

	/** Hint: Moonville was just toggled — search for widget and wire tabs to this tab. */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	void NotifyMoonvilleToggled(EZP_InventoryTab DesiredTab = EZP_InventoryTab::Inventory);

	/** Switch to a specific tab without opening/closing. */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	void SwitchToTab(EZP_InventoryTab Tab);

	/** Is the menu currently open? (reactive — based on Moonville widget presence) */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	bool IsMenuOpen() const { return bIsOpen; }

	/** Which tab is currently active? */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	EZP_InventoryTab GetCurrentTab() const { return CurrentTab; }

	/** Cycle to the next (+1) or previous (-1) tab. Wraps around. */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	void CycleTab(int32 Direction);

	/** Select and display a specific note by index. */
	UFUNCTION(BlueprintCallable, Category = "InventoryTab")
	void SelectNote(int32 NoteIndex);

	/** Bind to Grace's components (MapComp, NoteComp). Call once after creation. */
	void BindToCharacter(AZP_GraceCharacter* Character);

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	bool bIsOpen = false;
	bool bUIBuilt = false;
	bool bTabsWired = false;
	bool bSearchForWidget = false;  // Set by NotifyMoonvilleToggled, cleared after search
	EZP_InventoryTab CurrentTab = EZP_InventoryTab::Inventory;
	EZP_InventoryTab PendingTab = EZP_InventoryTab::Inventory;

	// --- Cached component refs ---
	UPROPERTY()
	TWeakObjectPtr<AZP_GraceCharacter> CachedCharacter;

	UPROPERTY()
	TWeakObjectPtr<UZP_MapComponent> CachedMapComp;

	UPROPERTY()
	TWeakObjectPtr<UZP_NoteComponent> CachedNoteComp;

	UPROPERTY()
	TWeakObjectPtr<AZP_MapVolume> CachedVolume;

	// --- Root canvas (minimal — widget is a non-visual controller) ---
	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;

	// --- Moonville widget (found in viewport, NOT created by us) ---
	UPROPERTY()
	TObjectPtr<UUserWidget> MoonvilleWidget;

	// --- Found inside Moonville widget (via GetWidgetFromName) ---
	UPROPERTY()
	TObjectPtr<UPanelWidget> TabHeaderPanel;  // User-placed "TabHeader"

	UPROPERTY()
	TObjectPtr<UImage> MoonvilleMapImage;  // User-placed "MapImage"

	// --- Moonville content widgets to hide on non-Inventory tabs ---
	UPROPERTY()
	TArray<TObjectPtr<UWidget>> InventoryContentWidgets;

	// --- Tab button row (HorizontalBox inside TabHeader VerticalBox) ---
	UPROPERTY()
	TObjectPtr<UHorizontalBox> TabButtonRow;

	// --- Tab buttons (created dynamically, added to TabButtonRow) ---
	UPROPERTY()
	TObjectPtr<UButton> MapTabButton;

	UPROPERTY()
	TObjectPtr<UButton> InventoryTabButton;

	UPROPERTY()
	TObjectPtr<UButton> NotesTabButton;

	// --- Map display (created dynamically, added near MapImage) ---
	UPROPERTY()
	TObjectPtr<UImage> TabPlayerMarker;

	UPROPERTY()
	TObjectPtr<UTextBlock> TabAreaNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TabNoMapText;

	// --- Notes widget (created dynamically, replaces old placeholder) ---
	UPROPERTY()
	TObjectPtr<UZP_NotesWidget> NotesWidget;

	/** Index of the currently selected note in CollectedNotes. -1 = none. */
	int32 SelectedNoteIndex = -1;

	// --- UI Construction ---
	void BuildUI();
	UButton* CreateTabButton(const FString& Label);

	// --- Wiring ---

	/** Find Moonville's widget in the viewport by class. */
	void FindMoonvilleWidget();

	/** Remove previously injected widgets before re-wiring. */
	void CleanupInjectedWidgets();

	/** Find TabHeader, MapImage in the Moonville widget and create tab buttons. */
	void WireTabsIntoMoonvilleWidget();

	/** Find a UWidget* variable on the Moonville widget by Blueprint property name. */
	UWidget* FindMoonvilleWidgetRef(const FName& PropertyName) const;

	// --- Tab button handlers ---
	UFUNCTION()
	void OnMapTabClicked();

	UFUNCTION()
	void OnInventoryTabClicked();

	UFUNCTION()
	void OnNotesTabClicked();

	// --- Internal ---
	void UpdateTabButtonStyles();
	void RefreshMapDisplay();
	void SetInventoryContentVisibility(ESlateVisibility InVisibility);

	/** Converts world XY to map UV (Y-flipped for screen coordinates). */
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;
};
