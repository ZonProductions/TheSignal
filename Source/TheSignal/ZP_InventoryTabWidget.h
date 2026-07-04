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
#include "InputCoreTypes.h"
#include "UObject/SoftObjectPath.h"
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
	FVector2D AZP_TabMarkerSize = FVector2D(20.0f, 20.0f);

	/** Tint color of the player chevron marker on the map (currently green). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	FLinearColor AZP_PlayerMarkerColor = FLinearColor(0.0f, 1.0f, 0.3f, 1.0f);

	/** Color for the active tab button text. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab|Style")
	FLinearColor AZP_ActiveTabColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);

	/** Color for inactive tab button text. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab|Style")
	FLinearColor AZP_InactiveTabColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.6f);

	/** Font size of the injected MAP/INVENTORY/NOTES tab button labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	int32 AZP_TabButtonFontSize = 14;

	/** Font size of the area display-name header on the map tab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	int32 AZP_AreaNameFontSize = 20;

	/** Font size of the 'No map available' / 'Map not found yet' message text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	int32 AZP_NoMapFontSize = 18;

	/** Key that cycles to the previous tab while the menu is open (Enhanced Input is blocked in UI mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_TabCycleLeftKey = EKeys::Q;

	/** Key that cycles to the next tab while the menu is open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_TabCycleRightKey = EKeys::E;

	/** Player-facing label of the Map tab button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_MapTabLabel = FText::FromString(TEXT("MAP"));

	/** Player-facing label of the Inventory tab button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_InventoryTabLabel = FText::FromString(TEXT("INVENTORY"));

	/** Player-facing label of the Notes tab button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_NotesTabLabel = FText::FromString(TEXT("NOTES"));

	/** Player-facing message shown on the map tab when the player is outside any MapVolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_NoMapAvailableText = FText::FromString(TEXT("No map available"));

	/** Player-facing message shown when the player is in a mapped area but has not picked up that area's map item yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_MapNotFoundText = FText::FromString(TEXT("Map not found yet"));

	/** Player-facing area title shown when no map area can be resolved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Text")
	FText AZP_UnknownAreaText = FText::FromString(TEXT("Unknown Area"));

	/** Widget class for Moonville inventory (used to find it in viewport). */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab")
	TSubclassOf<UUserWidget> AZP_InventoryWidgetClass;

	/** Fallback asset path auto-loaded into InventoryWidgetClass when no class is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab")
	FSoftClassPath AZP_DefaultInventoryWidgetClassPath = FSoftClassPath(TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror.WBP_InventoryMenu_Horror_C"));

	/** Widget class for the Notes panel (WBP_Notes). Auto-loaded if not set. */
	UPROPERTY(EditDefaultsOnly, Category = "InventoryTab")
	TSubclassOf<UZP_NotesWidget> AZP_NotesWidgetClass;

	/** Fallback asset path auto-loaded into NotesWidgetClass when no class is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab")
	FSoftClassPath AZP_DefaultNotesWidgetClassPath = FSoftClassPath(TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes_C"));

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

	/** Visibility each content widget had when captured — restored on show
	 *  (never blanket Visible: it breaks Moonville's hit-testing). */
	TArray<ESlateVisibility> InventoryContentSavedVis;

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
