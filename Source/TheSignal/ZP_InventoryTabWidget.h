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
#include "Engine/DataTable.h"
#include "ZP_InventoryTabTypes.h"
#include "ZP_InventoryTabWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UCanvasPanel;
class UPanelWidget;
class UHorizontalBox;
class UOverlaySlot;
class UBorder;
class AZP_GraceCharacter;
class UZP_MapComponent;
class UZP_NoteComponent;
class AZP_MapVolume;
class UZP_NotesWidget;
class FZPTabCycleInputProcessor;

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

	/** Gamepad button that cycles to the PREVIOUS tab (left bumper / LB).
	 *  Separate from AZP_TabCycleLeftKey because gamepad presses never reach this widget's
	 *  NativeOnKeyDown while a menu is up — CommonUI consumes them at the Slate preprocessor
	 *  layer. These two keys are serviced by a dedicated index-0 preprocessor instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_TabCycleLeftGamepadKey = EKeys::Gamepad_LeftShoulder;

	/** Gamepad button that cycles to the NEXT tab (right bumper / RB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_TabCycleRightGamepadKey = EKeys::Gamepad_RightShoulder;

	// --- Map layout ---

	/** Height (px) of the tab header band. The map viewport starts below this, so the map
	 *  never bleeds up behind the MAP/INVENTORY/NOTES buttons. Used as a fallback only when the
	 *  real TabHeader geometry is not measurable yet — normally the band is measured live. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapHeaderHeight = 64.0f;

	/** Gap (px) between the header band / screen edges and the map viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapViewportMargin = 24.0f;

	/** Fill colour of the darkened band behind the tab buttons. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	FLinearColor AZP_TabHeaderBackgroundColor = FLinearColor(0.015f, 0.017f, 0.02f, 0.92f);

	/** Padding inside the tab header band. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	FMargin AZP_TabHeaderPadding = FMargin(12.0f, 8.0f);

	// --- Map controls ---

	/** Closest zoom-out. 1.0 = whole map fitted inside the viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapZoomMin = 1.0f;

	/** Furthest zoom-in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapZoomMax = 6.0f;

	/** Zoom multiplier applied per mouse-wheel notch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapZoomPerWheelNotch = 0.15f;

	/** Zoom rate (fraction/sec) at full trigger pull. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapZoomSpeedGamepad = 1.5f;

	/** Pan speed (px/sec, at zoom 1) at full left-stick deflection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapPanSpeedGamepad = 1400.0f;

	/** Stick deflection below this is ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapStickDeadzone = 0.2f;

	/** Trigger pull below this is ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	float AZP_MapTriggerDeadzone = 0.1f;

	/** Hold right mouse button and move to pan the map. Disabling restores right-click-to-close
	 *  behaviour on the Map tab (the pan grab has to swallow the right button to work). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Map")
	bool AZP_MapRightDragPan = true;

	/** Keyboard key: view the next floor/area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_MapFloorUpKey = EKeys::W;

	/** Keyboard key: view the previous floor/area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_MapFloorDownKey = EKeys::D;

	/** Gamepad button: view the next floor/area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_MapFloorUpGamepadKey = EKeys::Gamepad_DPad_Up;

	/** Gamepad button: view the previous floor/area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_MapFloorDownGamepadKey = EKeys::Gamepad_DPad_Down;

	/** Gamepad button: scroll the note CONTENT up while the Notes tab is up.
	 *  (List SELECTION is the left stick — see the tick handler.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_NotesScrollUpGamepadKey = EKeys::Gamepad_DPad_Up;

	/** Gamepad button: scroll the note CONTENT down while the Notes tab is up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	FKey AZP_NotesScrollDownGamepadKey = EKeys::Gamepad_DPad_Down;

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

	// --- Backdrop (Map + Notes tabs) ---

	/** Full-screen texture drawn BEHIND the map viewport / notes panel — the same grunge the
	 *  inventory tab gets from Moonville's own BackgroundGrungeImage (which lives inside
	 *  InventoryCanvas and is collapsed on the other tabs — that is why Map/Notes had none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Style")
	FSoftObjectPath AZP_TabBackdropTexturePath = FSoftObjectPath(TEXT("/Game/InventorySystemPro/ExampleContent/Common/Textures/Inventory/T_BackgroundGrunge.T_BackgroundGrunge"));

	// --- Map legend (Map tab only) ---
	// 1:1 with the inventory tab's CommonBoundActionBar: entries are built from the SAME
	// footer-button widget the bar spawns (WBP_FooterButton2_Horror = CommonActionWidget glyph +
	// CommonTextBlock label), archetype-copied so style/font/colour are identical, and the glyphs
	// come from the same CommonInput controller data (BP_InventoryControllerData_KBM/Xbox).

	/** The action bar's entry widget — glyph + label templates are lifted from its tree. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Legend")
	FSoftClassPath AZP_LegendEntryClassPath = FSoftClassPath(TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FooterButton2_Horror.WBP_FooterButton2_Horror_C"));

	/** CommonUI action rows (keys + display names) for the legend. Row keys/labels are edited in
	 *  this table, not in code. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Legend")
	FSoftObjectPath AZP_LegendActionsTablePath = FSoftObjectPath(TEXT("/Game/Core/UI/DT_MapLegendActions.DT_MapLegendActions"));

	/** One legend entry per element; each element = comma-separated row names from the table.
	 *  Multiple rows in one entry render multiple glyphs beside one label (the label comes from
	 *  the FIRST row's DisplayName) — e.g. "MapZoomIn,MapZoomOut" shows RT+LT (pad) or
	 *  wheel-up+wheel-down (KBM) next to a single "Zoom". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Legend")
	TArray<FString> AZP_LegendEntries = { TEXT("MapZoomIn,MapZoomOut"), TEXT("MapMove"), TEXT("MapFloor") };

	/** CommonInput controller data the glyph BRUSHES are read from — the same assets that art the
	 *  inventory action bar. Brushes are resolved directly in C++ (TryGetInputBrush) and swapped by
	 *  UZP_GlyphDeviceSubsystem, so the legend does not depend on CommonUI's runtime resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Legend")
	FSoftClassPath AZP_LegendKBMControllerDataClassPath = FSoftClassPath(TEXT("/Game/InventorySystemPro/Blueprints/Input/Common/BP_InventoryControllerData_KBM.BP_InventoryControllerData_KBM_C"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Legend")
	FSoftClassPath AZP_LegendPadControllerDataClassPath = FSoftClassPath(TEXT("/Game/InventorySystemPro/Blueprints/Input/Common/BP_InventoryControllerData_Xbox.BP_InventoryControllerData_Xbox_C"));

	// --- Notes controls ---

	/** Seconds between selection steps while the left stick is held deflected on the Notes tab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	float AZP_NotesStickRepeatInterval = 0.28f;

	/** Pixels the note CONTENT scrolls per d-pad press/repeat on the Notes tab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InventoryTab|Input")
	float AZP_NotesContentScrollStep = 140.0f;

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
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void BeginDestroy() override;

private:
	friend class FZPTabCycleInputProcessor;

	/** Called by the preprocessor (and NativeOnKeyDown) — one tab step per physical press.
	 *  Returns true if the key was a tab-cycle key and was acted on (i.e. consume it). */
	bool HandleTabCycleKey(const FKey& Key);

	/** Map-tab-only keys (floor up/down). Returns true when consumed. */
	bool HandleMapKey(const FKey& Key);

	/** Notes-tab-only keys (d-pad content scrolling). Repeat events allowed — hold to scroll. */
	bool HandleNotesKey(const FKey& Key);

	/** True while the Map tab is showing and its viewport exists — gates map input capture. */
	bool IsMapInputActive() const;

	/** True while the Notes tab is showing — gates notes stick/d-pad capture. */
	bool IsNotesInputActive() const;

	// --- Map view (zoom / pan / floor) ---

	/** Rebuild the map viewport rect: full canvas inset by the measured header band + margin. */
	void UpdateMapLayout();

	/** Push MapZoom/MapPan onto the map surface + player marker. */
	void ApplyMapView();

	/** Back to fitted-and-centred. Called when the Map tab opens and on every floor change. */
	void ResetMapView();

	/** Step to another discovered area. Wraps. Order is arbitrary for now (sorted by ID). */
	void CycleMapFloor(int32 Direction);

	/** Discovered areas that have a volume, sorted by ID so cycling is at least deterministic. */
	void GetCycleableAreas(TArray<FName>& OutAreas) const;

	/** Area currently DISPLAYED. None = follow the player's own area. */
	FName ViewedAreaID;

	float MapZoom = 1.0f;
	FVector2D MapPan = FVector2D::ZeroVector;

	/** Size the map is drawn at when fitted to the viewport at zoom 1 (aspect preserved). */
	FVector2D MapFittedSize = FVector2D::ZeroVector;

	/** Viewport size last laid out — pan clamping needs it. */
	FVector2D MapViewportSize = FVector2D::ZeroVector;

	/** True when the viewed area is the one the player is actually standing in. */
	bool bViewingPlayerArea = true;

	/** Register/unregister the gamepad preprocessor. Live only while the menu is open. */
	void RegisterGamepadTabProcessor();
	void UnregisterGamepadTabProcessor();

	TSharedPtr<FZPTabCycleInputProcessor> GamepadTabProcessor;

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

	/** Darkened band wrapping the tab buttons — gives the header its own area.
	 *  Sits INSIDE TabHeader, so we never need to know what TabHeader's parent is. */
	UPROPERTY()
	TObjectPtr<UBorder> TabHeaderBorder;

	/** Clipping window the map is drawn through. Child of MapImage's canvas, inset below the
	 *  header. Everything zoom/pan does is clipped to this, so the map cannot bleed into the
	 *  header or off-screen. */
	UPROPERTY()
	TObjectPtr<UCanvasPanel> MapViewport;

	/** The map itself. Our own image, NOT Moonville's MapImage — panning/zooming needs a widget
	 *  we fully own inside the clipping viewport. Moonville's MapImage stays collapsed. */
	UPROPERTY()
	TObjectPtr<UImage> MapSurface;

	/** Full-screen grunge behind the map/notes content (Map + Notes tabs only —
	 *  the inventory tab has Moonville's own copy inside InventoryCanvas). */
	UPROPERTY()
	TObjectPtr<UImage> TabBackdropImage;

	/** Bottom-right row of action-bar-style entries — the Map tab's control legend,
	 *  slotted to match Moonville's CommonBoundActionBar (the inventory tab's legend). */
	UPROPERTY()
	TObjectPtr<UHorizontalBox> MapLegendBox;

	/** Build the legend entries into MapLegendBox from the actions table + entry templates. */
	void BuildMapLegend();

	/** One legend glyph image + its per-device brushes, resolved at build time straight from the
	 *  CommonInput controller data assets. No runtime resolution left to go wrong. */
	struct FZPLegendGlyphImage
	{
		TWeakObjectPtr<UImage> Image;
		FSlateBrush KBMBrush;
		FSlateBrush PadBrush;
		bool bHasKBM = false;
		bool bHasPad = false;
	};
	TArray<FZPLegendGlyphImage> LegendGlyphImages;

	/** Device the glyphs currently show. Re-applied when UZP_GlyphDeviceSubsystem flips. */
	bool bLegendGlyphsGamepad = false;

	/** Push the right per-device brush onto every legend glyph image. */
	void ApplyLegendDeviceBrushes(bool bForce);

	// Notes stick navigation state (step-repeat while deflected).
	float NotesStickRepeatT = 0.0f;
	bool bNotesStickHeld = false;

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
