// Copyright The Signal. All Rights Reserved.

#include "ZP_InventoryTabWidget.h"
#include "ZP_MapComponent.h"
#include "ZP_NoteComponent.h"
#include "ZP_NotesWidget.h"
#include "ZP_MapVolume.h"
#include "ZP_GlyphDeviceSubsystem.h"
#include "ZP_GraceCharacter.h"
#include "ZP_ObjectiveSubsystem.h"
#include "Engine/GameInstance.h"

#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "UObject/UnrealType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "HAL/IConsoleManager.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "CommonTextBlock.h"
#include "CommonInputBaseTypes.h"
#include "Engine/DataTable.h"

static TAutoConsoleVariable<int32> CVarZPTabCycleGamepad(
	TEXT("zp.TabCycle.Gamepad"), 1,
	TEXT("1 = L/R bumper cycles the inventory menu tabs (Map/Inventory/Notes) via a Slate input preprocessor. 0 = keyboard only."));

/**
 * FZPTabCycleInputProcessor
 *
 * WHY THIS EXISTS AND NativeOnKeyDown IS NOT ENOUGH:
 *   While a menu is up, CommonUI's own Slate input preprocessor CONSUMES gamepad key events
 *   before they are routed to any widget or to the PlayerController. That is the same
 *   documented failure that forced UZP_GlyphDeviceSubsystem's detection down to the
 *   preprocessor layer (2026-08-05, log-proven). Q/E work today because real keyboard keys are
 *   not consumed that way; the bumpers never arrive at all — neither at
 *   AZP_GraceCharacter::Input_TabCycleRight (Enhanced Input is dead in UI input mode) nor at
 *   UZP_InventoryTabWidget::NativeOnKeyDown.
 *
 *   Registering at index 0 puts us ahead of CommonUI, so we see the raw press. Unlike the glyph
 *   processor we DO consume (return true) — but only for the two configured bumper keys, and
 *   only while the tab menu is open and wired. Everything else passes through untouched, and the
 *   processor is unregistered the moment the menu closes.
 */
class FZPTabCycleInputProcessor : public IInputProcessor
{
public:
	explicit FZPTabCycleInputProcessor(UZP_InventoryTabWidget* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float, FSlateApplication&, TSharedRef<ICursor>) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		UZP_InventoryTabWidget* W = Owner.Get();
		if (!W)
		{
			return false;
		}
		// Tab cycling / floor stepping reject auto-repeat (one step per press); notes list
		// navigation ACCEPTS it so holding the d-pad scrolls through a long list.
		if (!InKeyEvent.IsRepeat() &&
			(W->HandleTabCycleKey(InKeyEvent.GetKey()) || W->HandleMapKey(InKeyEvent.GetKey())))
		{
			return true;
		}
		return W->HandleNotesKey(InKeyEvent.GetKey());
	}

	/** Analog events only fire when the value CHANGES, so we latch the newest value here and
	 *  the widget integrates it per tick. Otherwise a held trigger would zoom exactly once. */
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InEvent) override
	{
		UZP_InventoryTabWidget* W = Owner.Get();
		if (!W || (!W->IsMapInputActive() && !W->IsNotesInputActive()))
		{
			return false;
		}
		const FKey Key = InEvent.GetKey();
		const float V = InEvent.GetAnalogValue();
		if (Key == EKeys::Gamepad_LeftTriggerAxis)       { LeftTrigger = V;  return true; }
		if (Key == EKeys::Gamepad_RightTriggerAxis)      { RightTrigger = V; return true; }
		if (Key == EKeys::Gamepad_LeftX)                 { StickX = V;       return true; }
		if (Key == EKeys::Gamepad_LeftY)                 { StickY = V;       return true; }
		return false;
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp,
		const FPointerEvent& InWheelEvent, const FPointerEvent*) override
	{
		UZP_InventoryTabWidget* W = Owner.Get();
		if (!W || !W->IsMapInputActive())
		{
			return false;
		}
		WheelAccum += InWheelEvent.GetWheelDelta();
		return true;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& InEvent) override
	{
		UZP_InventoryTabWidget* W = Owner.Get();
		if (!W || !W->AZP_MapRightDragPan || !W->IsMapInputActive() ||
			InEvent.GetEffectingButton() != EKeys::RightMouseButton)
		{
			return false;
		}
		bRightDown = true;
		return true; // swallow, or Moonville closes the menu on right-click
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& InEvent) override
	{
		if (!bRightDown || InEvent.GetEffectingButton() != EKeys::RightMouseButton)
		{
			return false;
		}
		bRightDown = false;
		return true;
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& InEvent) override
	{
		if (!bRightDown)
		{
			return false;
		}
		UZP_InventoryTabWidget* W = Owner.Get();
		if (!W || !W->IsMapInputActive())
		{
			return false;
		}
		DragAccum += InEvent.GetCursorDelta();
		return true;
	}

	/** Consumed by the widget each tick. */
	float ConsumeWheel()          { const float V = WheelAccum; WheelAccum = 0.0f; return V; }
	FVector2D ConsumeDrag()       { const FVector2D V = DragAccum; DragAccum = FVector2D::ZeroVector; return V; }

	float LeftTrigger = 0.0f;
	float RightTrigger = 0.0f;
	float StickX = 0.0f;
	float StickY = 0.0f;

private:
	TWeakObjectPtr<UZP_InventoryTabWidget> Owner;
	float WheelAccum = 0.0f;
	FVector2D DragAccum = FVector2D::ZeroVector;
	bool bRightDown = false;
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool UZP_InventoryTabWidget::Initialize()
{
	bool bSuccess = Super::Initialize();
	if (bSuccess)
	{
		BuildUI();
	}
	return bSuccess;
}

void UZP_InventoryTabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Auto-load Moonville widget class for viewport searching
	if (!AZP_InventoryWidgetClass)
	{
		AZP_InventoryWidgetClass = LoadClass<UUserWidget>(nullptr,
			*AZP_DefaultInventoryWidgetClassPath.ToString());
	}

	// Auto-load Notes widget class
	if (!AZP_NotesWidgetClass)
	{
		AZP_NotesWidgetClass = LoadClass<UZP_NotesWidget>(nullptr,
			*AZP_DefaultNotesWidgetClassPath.ToString());
	}

	// Non-visual controller but needs to receive key events for tab cycling.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	bIsOpen = false;
}

bool UZP_InventoryTabWidget::HandleTabCycleKey(const FKey& Key)
{
	if (!bIsOpen || !bTabsWired)
	{
		return false;
	}

	const bool bLeft = (Key == AZP_TabCycleLeftKey) ||
		(AZP_TabCycleLeftGamepadKey.IsValid() && Key == AZP_TabCycleLeftGamepadKey);
	const bool bRight = (Key == AZP_TabCycleRightKey) ||
		(AZP_TabCycleRightGamepadKey.IsValid() && Key == AZP_TabCycleRightGamepadKey);

	if (bLeft)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Cycling LEFT (%s)"), *Key.ToString());
		CycleTab(-1);
		return true;
	}
	if (bRight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Cycling RIGHT (%s)"), *Key.ToString());
		CycleTab(1);
		return true;
	}
	return false;
}

FReply UZP_InventoryTabWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] NativeOnKeyDown: Key=%s bIsOpen=%d bTabsWired=%d HasFocus=%d"),
		*Key.ToString(), bIsOpen, bTabsWired, HasKeyboardFocus());

	// Gamepad presses normally never get here (CommonUI consumes them upstream) — the
	// preprocessor handles those. This path still covers the keyboard keys, and covers the
	// bumpers too if no one upstream ate them.
	if (!InKeyEvent.IsRepeat() && (HandleTabCycleKey(Key) || HandleMapKey(Key)))
	{
		return FReply::Handled();
	}
	if (HandleNotesKey(Key))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ---------------------------------------------------------------------------
// Gamepad tab cycling (Slate preprocessor — see FZPTabCycleInputProcessor)
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::RegisterGamepadTabProcessor()
{
	if (GamepadTabProcessor.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	if (!CVarZPTabCycleGamepad.GetValueOnGameThread())
	{
		return;
	}

	GamepadTabProcessor = MakeShared<FZPTabCycleInputProcessor>(this);
	// INDEX 0 — must be ahead of CommonUI's preprocessor, which consumes gamepad keys while a
	// menu is up (same reason UZP_GlyphDeviceSubsystem registers at 0).
	FSlateApplication::Get().RegisterInputPreProcessor(GamepadTabProcessor, 0);
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Gamepad tab processor registered (%s / %s)"),
		*AZP_TabCycleLeftGamepadKey.ToString(), *AZP_TabCycleRightGamepadKey.ToString());
}

void UZP_InventoryTabWidget::UnregisterGamepadTabProcessor()
{
	if (!GamepadTabProcessor.IsValid())
	{
		return;
	}
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(GamepadTabProcessor);
	}
	GamepadTabProcessor.Reset();
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Gamepad tab processor unregistered"));
}

void UZP_InventoryTabWidget::NativeDestruct()
{
	UnregisterGamepadTabProcessor();
	Super::NativeDestruct();
}

void UZP_InventoryTabWidget::BeginDestroy()
{
	// Belt and braces: the processor holds a weak ptr, but leaving a dead one registered would
	// keep consuming bumper presses for the rest of the session.
	UnregisterGamepadTabProcessor();
	Super::BeginDestroy();
}

void UZP_InventoryTabWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// --- Periodic state dump (every ~2 seconds) ---
	static float DebugTimer = 0.0f;
	DebugTimer += InDeltaTime;
	if (DebugTimer >= 2.0f)
	{
		DebugTimer = 0.0f;
		const bool bWidgetValid = MoonvilleWidget != nullptr;
		const bool bInVP = bWidgetValid && MoonvilleWidget->IsInViewport();
		UE_LOG(LogTemp, Log, TEXT("[INVTAB-TICK] bIsOpen=%d bTabsWired=%d bSearchForWidget=%d MoonvilleWidget=%s InViewport=%d CurrentTab=%d PendingTab=%d"),
			bIsOpen, bTabsWired, bSearchForWidget,
			bWidgetValid ? *MoonvilleWidget->GetName() : TEXT("null"),
			bInVP, (int32)CurrentTab, (int32)PendingTab);
		if (bWidgetValid)
		{
			UE_LOG(LogTemp, Log, TEXT("[INVTAB-TICK]   MapImage=%s TabHeader=%s TabButtonRow=%s InvContentWidgets=%d"),
				MoonvilleMapImage ? TEXT("valid") : TEXT("null"),
				TabHeaderPanel ? TEXT("valid") : TEXT("null"),
				TabButtonRow ? TEXT("valid") : TEXT("null"),
				InventoryContentWidgets.Num());
			if (MoonvilleMapImage)
			{
				UE_LOG(LogTemp, Log, TEXT("[INVTAB-TICK]   MapImage Vis=%d InParent=%s"),
					(int32)MoonvilleMapImage->GetVisibility(),
					MoonvilleMapImage->GetParent() ? *MoonvilleMapImage->GetParent()->GetName() : TEXT("null"));
			}
		}
	}

	// --- Search for Moonville widget when requested ---
	// Also re-search if we have a stale ref (widget removed from viewport but not GC'd)
	if (bSearchForWidget)
	{
		if (!MoonvilleWidget || !MoonvilleWidget->IsInViewport())
		{
			MoonvilleWidget = nullptr;
			UE_LOG(LogTemp, Log, TEXT("[INVTAB] Searching for Moonville widget..."));
			FindMoonvilleWidget();
			bSearchForWidget = (MoonvilleWidget == nullptr); // Keep searching until found
			UE_LOG(LogTemp, Log, TEXT("[INVTAB] Search result: MoonvilleWidget=%s, keepSearching=%d"),
				MoonvilleWidget ? *MoonvilleWidget->GetName() : TEXT("null"), bSearchForWidget);
		}
		else
		{
			// Already have a valid in-viewport widget
			bSearchForWidget = false;
		}
	}

	// --- Detect Moonville open/close reactively ---
	const bool bMoonvilleInViewport = MoonvilleWidget && MoonvilleWidget->IsInViewport();

	if (bMoonvilleInViewport && !bIsOpen)
	{
		// Moonville just opened — wire tabs
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB] >>> OPEN DETECTED: MoonvilleWidget=%s, bTabsWired=%d, PendingTab=%d"),
			*MoonvilleWidget->GetName(), bTabsWired, (int32)PendingTab);
		bIsOpen = true;

		// Pause the world while the inventory is open — same SetPause the pause menu uses (dev request).
		// Hooked here, not on the open input, so it stays synced to Moonville's ACTUAL viewport state
		// and unpauses no matter how the menu closes (Tab / Map / right-click / EGUI). NativeTick keeps
		// running while paused (Slate-driven), so the close branch below still fires.
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetPause(true);
		}
		if (!bTabsWired)
		{
			WireTabsIntoMoonvilleWidget();
		}
		if (bTabsWired)
		{
			SwitchToTab(PendingTab);

			// Grab keyboard focus so NativeOnKeyDown fires for Q/E tab cycling
			SetKeyboardFocus();
			UE_LOG(LogTemp, Warning, TEXT("[INVTAB] SetKeyboardFocus called — HasFocus=%d"),
				HasKeyboardFocus());

			// Focus is not enough for the bumpers: CommonUI eats gamepad keys before any widget
			// sees them, so L/R go through a Slate preprocessor that only lives while we're open.
			RegisterGamepadTabProcessor();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[INVTAB] >>> OPEN but wiring FAILED — tabs will not show"));
		}
	}
	else if (!bMoonvilleInViewport && bIsOpen)
	{
		// Moonville closed (right-click, ESC, Tab, etc.) — clean up
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB] >>> CLOSE DETECTED: MoonvilleWidget=%s"),
			MoonvilleWidget ? *MoonvilleWidget->GetName() : TEXT("null/GCed"));

		if (bTabsWired)
		{
			SetInventoryContentVisibility(ESlateVisibility::Visible);
			if (MapViewport) MapViewport->SetVisibility(ESlateVisibility::Collapsed);
			if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
			if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
			if (TabBackdropImage) TabBackdropImage->SetVisibility(ESlateVisibility::Collapsed);
			if (MapLegendBox) MapLegendBox->SetVisibility(ESlateVisibility::Collapsed);
			if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
			if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
			if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		bIsOpen = false;

		// Stop intercepting the bumpers the instant the menu is gone.
		UnregisterGamepadTabProcessor();

		// Inventory closed by any means — unpause (PlayerController::SetPause override restores
		// game input/cursor on unpause).
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetPause(false);
		}

		// Tab menu (Map/Inventory/Notes) closed → re-show the objective tracker for its timed window.
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UZP_ObjectiveSubsystem* ObjSys = GI->GetSubsystem<UZP_ObjectiveSubsystem>())
			{
				ObjSys->NotifyMenuClosed();
			}
		}

		bTabsWired = false;
		MoonvilleWidget = nullptr;
		MoonvilleMapImage = nullptr;
		TabHeaderPanel = nullptr;
		TabButtonRow = nullptr;
		MapTabButton = nullptr;
		InventoryTabButton = nullptr;
		NotesTabButton = nullptr;
		TabPlayerMarker = nullptr;
		TabAreaNameText = nullptr;
		TabNoMapText = nullptr;
		NotesWidget = nullptr;
		InventoryContentWidgets.Empty();
		InventoryContentSavedVis.Empty();
		SelectedNoteIndex = -1;

		// Sync GraceCharacter state (Moonville may have closed via right-click)
		if (CachedCharacter.IsValid())
		{
			CachedCharacter->bInventoryMenuOpen = false;
			CachedCharacter->bMapOpen = false;
		}

		UE_LOG(LogTemp, Warning, TEXT("[INVTAB] >>> CLOSE cleanup complete — all refs nulled"));
	}

	if (!bIsOpen) return;

	// --- Deferred wiring (Moonville widget may need a frame to construct) ---
	if (!bTabsWired && MoonvilleWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("[INVTAB] Deferred wiring attempt..."));
		WireTabsIntoMoonvilleWidget();
		if (bTabsWired)
		{
			SwitchToTab(PendingTab);
		}
		return;
	}

	// --- Tab cycling via raw key polling (Enhanced Input blocked in UI mode) ---
	if (bTabsWired)
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			if (PC->WasInputKeyJustPressed(AZP_TabCycleLeftKey))
			{
				CycleTab(-1);
			}
			else if (PC->WasInputKeyJustPressed(AZP_TabCycleRightKey))
			{
				CycleTab(1);
			}
		}
	}

	// --- Map view: layout, controls, marker ---
	if (CurrentTab == EZP_InventoryTab::Map)
	{
		// Swap glyph brushes when the player switches device (same signal as the EGUI prompts).
		ApplyLegendDeviceBrushes(/*bForce*/ false);

		// Keep the viewport parked under the header band (header height can change with
		// resolution/DPI, so it is re-measured rather than cached).
		UpdateMapLayout();

		// If the player walks into a different area while the menu is open and we are following
		// them (no manual floor selection), follow the change.
		if (ViewedAreaID.IsNone() && CachedMapComp.IsValid())
		{
			const FName PlayerArea = CachedMapComp->GetCurrentAreaID();
			if (CachedVolume.IsValid() && CachedVolume->AZP_AreaID != PlayerArea)
			{
				RefreshMapDisplay();
			}
		}

		// --- Continuous controls (integrate the latched analog state) ---
		if (GamepadTabProcessor.IsValid() && MapViewport && MapSurface)
		{
			const FVector2D ViewSize = MapViewport->GetCachedGeometry().GetLocalSize();

			// Zoom about the viewport centre so the point you are looking at stays put.
			float ZoomMul = 1.0f;

			const float Wheel = GamepadTabProcessor->ConsumeWheel();
			if (!FMath::IsNearlyZero(Wheel))
			{
				ZoomMul *= FMath::Pow(1.0f + AZP_MapZoomPerWheelNotch, Wheel);
			}

			const float RT = (FMath::Abs(GamepadTabProcessor->RightTrigger) > AZP_MapTriggerDeadzone)
				? GamepadTabProcessor->RightTrigger : 0.0f;
			const float LT = (FMath::Abs(GamepadTabProcessor->LeftTrigger) > AZP_MapTriggerDeadzone)
				? GamepadTabProcessor->LeftTrigger : 0.0f;
			const float TriggerAxis = RT - LT; // RT zooms in, LT zooms out
			if (!FMath::IsNearlyZero(TriggerAxis))
			{
				ZoomMul *= 1.0f + AZP_MapZoomSpeedGamepad * TriggerAxis * InDeltaTime;
			}

			if (!FMath::IsNearlyEqual(ZoomMul, 1.0f))
			{
				const float OldZoom = MapZoom;
				const float NewZoom = FMath::Clamp(MapZoom * ZoomMul, AZP_MapZoomMin, AZP_MapZoomMax);
				if (!FMath::IsNearlyEqual(NewZoom, OldZoom) && OldZoom > 0.0001f)
				{
					// Keep the viewport centre anchored: solve pan so the map point currently at
					// the centre is still at the centre after the scale change.
					const FVector2D Centre = ViewSize * 0.5f;
					const float Ratio = NewZoom / OldZoom;
					MapPan = Centre - (Centre - MapPan) * Ratio;
					MapZoom = NewZoom;
				}
			}

			// Pan — right-drag (1:1 with the cursor) and left stick.
			MapPan += GamepadTabProcessor->ConsumeDrag();

			FVector2D Stick(GamepadTabProcessor->StickX, GamepadTabProcessor->StickY);
			if (Stick.SizeSquared() > AZP_MapStickDeadzone * AZP_MapStickDeadzone)
			{
				// Stick Y is +up; screen Y is +down. Pushing the stick moves the VIEW, so the map
				// itself travels the opposite way.
				MapPan.X -= Stick.X * AZP_MapPanSpeedGamepad * InDeltaTime;
				MapPan.Y += Stick.Y * AZP_MapPanSpeedGamepad * InDeltaTime;
			}
		}

		ApplyMapView();

		static int32 MapLogCounter = 0;
		if (MapLogCounter++ % 120 == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[MAP-TAB] area=%s zoom=%.2f pan=(%.0f,%.0f) fitted=(%.0f,%.0f) view=(%.0f,%.0f)"),
				CachedVolume.IsValid() ? *CachedVolume->AZP_AreaID.ToString() : TEXT("none"),
				MapZoom, MapPan.X, MapPan.Y, MapFittedSize.X, MapFittedSize.Y,
				MapViewportSize.X, MapViewportSize.Y);
		}
	}
	// --- Notes: left stick steps the SELECTION (analog, so it's integrated here per tick) ---
	else if (CurrentTab == EZP_InventoryTab::Notes && NotesWidget && GamepadTabProcessor.IsValid())
	{
		const float StickY = GamepadTabProcessor->StickY;
		if (FMath::Abs(StickY) > AZP_MapStickDeadzone)
		{
			NotesStickRepeatT -= InDeltaTime;
			if (!bNotesStickHeld || NotesStickRepeatT <= 0.0f)
			{
				// Stick up (+Y) = previous note (up the list), down = next.
				NotesWidget->NavigateSelection(StickY > 0.0f ? -1 : 1);
				NotesStickRepeatT = AZP_NotesStickRepeatInterval;
				bNotesStickHeld = true;
			}
		}
		else
		{
			bNotesStickHeld = false;
			NotesStickRepeatT = 0.0f;
		}
	}
}

// ---------------------------------------------------------------------------
// UI Construction (minimal — just root canvas so widget tree isn't empty)
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::BuildUI()
{
	if (bUIBuilt) return;
	if (!WidgetTree) return;
	bUIBuilt = true;

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootCanvas;
}

UButton* UZP_InventoryTabWidget::CreateTabButton(const FString& Label)
{
	UButton* Btn = NewObject<UButton>(this);

	// Make button background fully transparent (alpha=0 hides the default gray)
	Btn->SetBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(FText::FromString(Label));
	Text->SetColorAndOpacity(FSlateColor(AZP_InactiveTabColor));
	Text->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = AZP_TabButtonFontSize;
		Text->SetFont(Font);
	}

	Btn->AddChild(Text);

	return Btn;
}

// ---------------------------------------------------------------------------
// Finding Moonville's widget
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::FindMoonvilleWidget()
{
	if (!AZP_InventoryWidgetClass || !GetWorld()) return;

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Found, AZP_InventoryWidgetClass, false);

	// Only accept widgets that are ACTUALLY in the viewport — stale removed
	// widgets still exist as UObjects until GC, but IsInViewport() returns false.
	MoonvilleWidget = nullptr;
	for (UUserWidget* W : Found)
	{
		if (W && W->IsInViewport())
		{
			MoonvilleWidget = W;
			UE_LOG(LogTemp, Warning, TEXT("[INVTAB] Found Moonville widget IN VIEWPORT: %s"), *W->GetName());
			return;
		}
		else if (W)
		{
			UE_LOG(LogTemp, Log, TEXT("[INVTAB] Skipping stale Moonville widget (not in viewport): %s"), *W->GetName());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[INVTAB] No Moonville widget in viewport (found %d total, all stale)"), Found.Num());
}

// ---------------------------------------------------------------------------
// Wiring — inject tabs into the Moonville widget
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::CleanupInjectedWidgets()
{
	// Remove previously injected widgets from the Moonville widget tree
	if (TabButtonRow) { TabButtonRow->RemoveFromParent(); TabButtonRow = nullptr; }
	if (MapTabButton) { MapTabButton->RemoveFromParent(); MapTabButton = nullptr; }
	if (InventoryTabButton) { InventoryTabButton->RemoveFromParent(); InventoryTabButton = nullptr; }
	if (NotesTabButton) { NotesTabButton->RemoveFromParent(); NotesTabButton = nullptr; }
	if (TabHeaderBorder) { TabHeaderBorder->RemoveFromParent(); TabHeaderBorder = nullptr; }
	if (TabPlayerMarker) { TabPlayerMarker->RemoveFromParent(); TabPlayerMarker = nullptr; }
	if (MapSurface) { MapSurface->RemoveFromParent(); MapSurface = nullptr; }
	if (MapViewport) { MapViewport->RemoveFromParent(); MapViewport = nullptr; }
	if (TabBackdropImage) { TabBackdropImage->RemoveFromParent(); TabBackdropImage = nullptr; }
	if (MapLegendBox) { MapLegendBox->RemoveFromParent(); MapLegendBox = nullptr; }
	LegendGlyphImages.Reset();
	if (TabAreaNameText) { TabAreaNameText->RemoveFromParent(); TabAreaNameText = nullptr; }
	if (TabNoMapText) { TabNoMapText->RemoveFromParent(); TabNoMapText = nullptr; }
	if (NotesWidget) { NotesWidget->RemoveFromParent(); NotesWidget = nullptr; }

	InventoryContentWidgets.Empty();
	InventoryContentSavedVis.Empty();
	TabHeaderPanel = nullptr;
	MoonvilleMapImage = nullptr;
}

void UZP_InventoryTabWidget::WireTabsIntoMoonvilleWidget()
{
	if (bTabsWired || !MoonvilleWidget) return;

	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] === START WIRING into %s ==="), *MoonvilleWidget->GetName());

	// Clean up any stale injected widgets from a previous wiring
	CleanupInjectedWidgets();

	// Find user-placed widgets by name
	TabHeaderPanel = Cast<UPanelWidget>(MoonvilleWidget->GetWidgetFromName(TEXT("TabHeader")));
	MoonvilleMapImage = Cast<UImage>(MoonvilleWidget->GetWidgetFromName(TEXT("MapImage")));

	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] TabHeader=%s (class=%s), MapImage=%s"),
		TabHeaderPanel ? *TabHeaderPanel->GetName() : TEXT("NOT FOUND"),
		TabHeaderPanel ? *TabHeaderPanel->GetClass()->GetName() : TEXT("n/a"),
		MoonvilleMapImage ? *MoonvilleMapImage->GetName() : TEXT("NOT FOUND"));

	if (!TabHeaderPanel)
	{
		// Dump all widget names in the Moonville widget for debugging
		if (MoonvilleWidget->WidgetTree)
		{
			MoonvilleWidget->WidgetTree->ForEachWidget([](UWidget* W) {
				UE_LOG(LogTemp, Log, TEXT("[INVTAB-WIRE]   Child: %s (%s)"), *W->GetName(), *W->GetClass()->GetName());
			});
		}
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] 'TabHeader' not found in Moonville widget — WIRING ABORTED"));
		return;
	}

	// --- Create the tab header band ---
	// The buttons go inside a UBorder rather than straight into TabHeader, so the tabs get their
	// own darkened area. Wrapping INSIDE TabHeader (instead of adding a backing panel behind it)
	// means we never have to know or touch whatever TabHeader's parent is.
	TabHeaderBorder = NewObject<UBorder>(this);
	TabHeaderBorder->SetBrushColor(AZP_TabHeaderBackgroundColor);
	TabHeaderBorder->SetPadding(AZP_TabHeaderPadding);
	TabHeaderBorder->SetHorizontalAlignment(HAlign_Fill);
	TabHeaderBorder->SetVerticalAlignment(VAlign_Fill);
	TabHeaderPanel->AddChild(TabHeaderBorder);

	TabButtonRow = NewObject<UHorizontalBox>(this);
	TabHeaderBorder->AddChild(TabButtonRow);

	MapTabButton = CreateTabButton(AZP_MapTabLabel.ToString());
	TabButtonRow->AddChild(MapTabButton);
	MapTabButton->OnClicked.AddDynamic(this, &UZP_InventoryTabWidget::OnMapTabClicked);

	InventoryTabButton = CreateTabButton(AZP_InventoryTabLabel.ToString());
	TabButtonRow->AddChild(InventoryTabButton);
	InventoryTabButton->OnClicked.AddDynamic(this, &UZP_InventoryTabWidget::OnInventoryTabClicked);

	NotesTabButton = CreateTabButton(AZP_NotesTabLabel.ToString());
	TabButtonRow->AddChild(NotesTabButton);
	NotesTabButton->OnClicked.AddDynamic(this, &UZP_InventoryTabWidget::OnNotesTabClicked);

	// Set equal fill sizing in the HorizontalBox
	for (UButton* Btn : { MapTabButton.Get(), InventoryTabButton.Get(), NotesTabButton.Get() })
	{
		if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Btn->Slot))
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HSlot->SetPadding(FMargin(4.0f, 0.0f));
		}
	}

	// --- Find Moonville inventory content widgets to hide on non-Inventory tabs ---
	static const FName ContentRefNames[] = {
		"InventoryCanvasRef",
		"InspectionCanvasRef",
		"ItemDescriptionRef",
		"CurrencyTextRef",
		"ContextMenuInputBlockerRef",
	};

	for (const FName& PropName : ContentRefNames)
	{
		if (UWidget* W = FindMoonvilleWidgetRef(PropName))
		{
			InventoryContentWidgets.Add(W);
		}
	}

	// Also find ShortcutMenuRef
	if (UWidget* W = FindMoonvilleWidgetRef("ShortcutMenuRef"))
	{
		InventoryContentWidgets.Add(W);
	}

	// Also find widgets by name in the Moonville tree (ShortcutCross, MenuSwitcher, etc.)
	for (const TCHAR* Name : { TEXT("ItemShortcutCross"), TEXT("MenuSwitcher"),
		TEXT("InventoryCanvas"), TEXT("ShortcutMenu") })
	{
		if (UWidget* W = MoonvilleWidget->GetWidgetFromName(Name))
		{
			InventoryContentWidgets.AddUnique(W);
		}
	}

	// Fallback: try finding by ref names if the above didn't capture enough
	if (InventoryContentWidgets.Num() == 0)
	{
		for (const TCHAR* Name : { TEXT("InventoryCanvasRef"), TEXT("ShortcutMenuRef") })
		{
			if (UWidget* W = MoonvilleWidget->GetWidgetFromName(Name))
			{
				InventoryContentWidgets.AddUnique(W);
			}
		}
	}

	// Record Moonville's authored visibility for each captured widget —
	// restore must put THESE back, never blanket Visible: forcing hidden
	// modals (ItemShortcutCross) visible parks an invisible click-eater over
	// the item grid (session 63 bug: no hover/clicks on inventory items).
	InventoryContentSavedVis.Reset();
	for (UWidget* W : InventoryContentWidgets)
	{
		InventoryContentSavedVis.Add(W ? W->GetVisibility() : ESlateVisibility::Collapsed);
	}

	// --- Create player marker for map ---
	if (MoonvilleMapImage)
	{
		UPanelWidget* MapParent = MoonvilleMapImage->GetParent();
		if (MapParent)
		{
			// --- Backdrop: full-screen grunge behind map/notes content ---
			// The inventory tab's grunge is Moonville's own BackgroundGrungeImage inside
			// InventoryCanvas — collapsed with the rest of the inventory content on other tabs,
			// which is why Map/Notes sat on a bare background. Same texture, our own widget.
			TabBackdropImage = NewObject<UImage>(this);
			if (UTexture2D* Grunge = Cast<UTexture2D>(AZP_TabBackdropTexturePath.TryLoad()))
			{
				TabBackdropImage->SetBrushFromTexture(Grunge);
			}
			TabBackdropImage->SetVisibility(ESlateVisibility::Collapsed);
			MapParent->AddChild(TabBackdropImage);
			if (UCanvasPanelSlot* BSlot = Cast<UCanvasPanelSlot>(TabBackdropImage->Slot))
			{
				BSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				BSlot->SetOffsets(FMargin(0.0f));
				BSlot->SetZOrder(-1); // under the map viewport, notes panel and header
			}

			// --- Map legend: action-bar-style row, slotted where the inventory's bar sits ---
			MapLegendBox = NewObject<UHorizontalBox>(this);
			MapLegendBox->SetVisibility(ESlateVisibility::Collapsed);
			MapParent->AddChild(MapLegendBox);
			if (UCanvasPanelSlot* LSlot = Cast<UCanvasPanelSlot>(MapLegendBox->Slot))
			{
				bool bCopied = false;
				// Mirror the inventory legend's exact placement (CommonBoundActionBar).
				if (UWidget* Bar = MoonvilleWidget->GetWidgetFromName(TEXT("CommonBoundActionBar")))
				{
					if (UCanvasPanelSlot* BarSlot = Cast<UCanvasPanelSlot>(Bar->Slot))
					{
						LSlot->SetAnchors(BarSlot->GetAnchors());
						LSlot->SetOffsets(BarSlot->GetOffsets());
						LSlot->SetAlignment(BarSlot->GetAlignment());
						LSlot->SetAutoSize(true);
						bCopied = true;
					}
				}
				if (!bCopied)
				{
					// Fallback: bottom-right, matching the horror menu footer line.
					LSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
					LSlot->SetAlignment(FVector2D(1.0f, 1.0f));
					LSlot->SetPosition(FVector2D(-40.0f, -10.0f));
					LSlot->SetAutoSize(true);
				}
				LSlot->SetZOrder(5);
			}
			BuildMapLegend();

			// --- Map viewport: a clipping window the map is drawn through ---
			// Moonville's MapImage stays COLLAPSED from here on. It used to be the map surface,
			// but it was forced to anchors (0,0,1,1) with zero offsets every tick, i.e. the whole
			// screen — which is why the map ran up behind the header buttons. We draw into our own
			// image inside this clipping panel instead, so zoom/pan can never escape the frame.
			MapViewport = NewObject<UCanvasPanel>(this);
			MapViewport->SetClipping(EWidgetClipping::ClipToBounds);
			MapViewport->SetVisibility(ESlateVisibility::HitTestInvisible);
			MapParent->AddChild(MapViewport);
			if (UCanvasPanelSlot* VSlot = Cast<UCanvasPanelSlot>(MapViewport->Slot))
			{
				VSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				VSlot->SetOffsets(FMargin(0.0f));
				VSlot->SetZOrder(0);
			}

			MapSurface = NewObject<UImage>(this);
			MapSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
			MapViewport->AddChild(MapSurface);
			if (UCanvasPanelSlot* SSlot = Cast<UCanvasPanelSlot>(MapSurface->Slot))
			{
				SSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				SSlot->SetAlignment(FVector2D::ZeroVector);
				SSlot->SetAutoSize(false);
				SSlot->SetZOrder(0);
			}

			// Player marker — green triangle (arrow) that rotates with player direction
			TabPlayerMarker = NewObject<UImage>(this);
			TabPlayerMarker->SetColorAndOpacity(AZP_PlayerMarkerColor);
			TabPlayerMarker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

			// Create sleek chevron arrow: outlined V-shape pointing up
			UTexture2D* ArrowTex = UTexture2D::CreateTransient(32, 32, PF_B8G8R8A8);
			if (ArrowTex)
			{
				FTexture2DMipMap& Mip = ArrowTex->GetPlatformData()->Mips[0];
				Mip.BulkData.Lock(LOCK_READ_WRITE);
				uint8* Data = (uint8*)Mip.BulkData.Realloc(32 * 32 * 4);
				FMemory::Memzero(Data, 32 * 32 * 4);

				auto SetPixel = [&](int32 X, int32 Y, uint8 A = 255) {
					if (X >= 0 && X < 32 && Y >= 0 && Y < 32) {
						int32 Idx = (Y * 32 + X) * 4;
						Data[Idx] = Data[Idx+1] = Data[Idx+2] = 255;
						Data[Idx+3] = A;
					}
				};

				// Outer chevron (filled arrow with hollow interior)
				// Tip at Y=2, wings spread to Y=26
				int32 CX = 16;
				for (int32 Y = 2; Y <= 26; Y++)
				{
					float T = (float)(Y - 2) / 24.0f;
					int32 OuterW = FMath::RoundToInt(T * 13.0f);
					// Inner cutout starts at Y=10, creating the hollow V
					int32 InnerW = 0;
					if (Y > 10)
					{
						float IT = (float)(Y - 10) / 16.0f;
						InnerW = FMath::Max(0, FMath::RoundToInt(IT * 10.0f));
					}

					for (int32 X = CX - OuterW; X <= CX + OuterW; X++)
					{
						// Skip inner area to create hollow chevron
						if (InnerW > 0 && X > CX - InnerW && X < CX + InnerW)
							continue;
						SetPixel(X, Y);
					}
				}

				Mip.BulkData.Unlock();
				ArrowTex->UpdateResource();
				TabPlayerMarker->SetBrushFromTexture(ArrowTex);
			}
			TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
			// Inside the viewport, NOT the outer canvas — the marker has to pan, zoom and clip
			// with the map it is marking.
			MapViewport->AddChild(TabPlayerMarker);

			if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(TabPlayerMarker->Slot))
			{
				CSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				CSlot->SetAlignment(FVector2D::ZeroVector);
				CSlot->SetSize(AZP_TabMarkerSize);
				CSlot->SetAutoSize(false);
				CSlot->SetZOrder(10);
			}
			else if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(TabPlayerMarker->Slot))
			{
				OSlot->SetHorizontalAlignment(HAlign_Left);
				OSlot->SetVerticalAlignment(VAlign_Top);
			}

			// Area name text
			TabAreaNameText = NewObject<UTextBlock>(this);
			TabAreaNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			{
				FSlateFontInfo Font = TabAreaNameText->GetFont();
				Font.Size = AZP_AreaNameFontSize;
				TabAreaNameText->SetFont(Font);
			}
			TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
			MapParent->AddChild(TabAreaNameText);

			if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(TabAreaNameText->Slot))
			{
				OSlot->SetHorizontalAlignment(HAlign_Center);
				OSlot->SetVerticalAlignment(VAlign_Top);
			}

			// "No map" text
			TabNoMapText = NewObject<UTextBlock>(this);
			TabNoMapText->SetText(AZP_NoMapAvailableText);
			TabNoMapText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
			{
				FSlateFontInfo Font = TabNoMapText->GetFont();
				Font.Size = AZP_NoMapFontSize;
				TabNoMapText->SetFont(Font);
			}
			TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
			MapParent->AddChild(TabNoMapText);

			if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(TabNoMapText->Slot))
			{
				OSlot->SetHorizontalAlignment(HAlign_Center);
				OSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	// --- Notes widget ---
	if (AZP_NotesWidgetClass && MoonvilleMapImage && MoonvilleMapImage->GetParent())
	{
		NotesWidget = CreateWidget<UZP_NotesWidget>(this, AZP_NotesWidgetClass);
		if (NotesWidget)
		{
			NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
			MoonvilleMapImage->GetParent()->AddChild(NotesWidget);

			// Copy MapImage's exact CanvasPanelSlot so notes fits the same bordered rectangle
			if (UCanvasPanelSlot* MapSlot = Cast<UCanvasPanelSlot>(MoonvilleMapImage->Slot))
			{
				if (UCanvasPanelSlot* NotesSlot = Cast<UCanvasPanelSlot>(NotesWidget->Slot))
				{
					NotesSlot->SetAnchors(MapSlot->GetAnchors());
					NotesSlot->SetOffsets(MapSlot->GetOffsets());
					NotesSlot->SetAlignment(MapSlot->GetAlignment());
					NotesSlot->SetAutoSize(false); // Must be false so CanvasPanel constrains height — enables ScrollBox scrolling
					NotesSlot->SetZOrder(MapSlot->GetZOrder() + 1);
				}
			}

			if (CachedNoteComp.IsValid())
			{
				NotesWidget->BindToNoteComponent(CachedNoteComp.Get());
			}

			UE_LOG(LogTemp, Log, TEXT("[INVTAB-WIRE] NotesWidget created — copied MapImage slot for identical positioning"));
		}
	}
	else if (!AZP_NotesWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] AZP_NotesWidgetClass is null — notes tab will be empty"));
	}

	// Start with map/notes content hidden (Inventory is the default tab)
	if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);

	bTabsWired = true;

	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] === WIRING SUCCESS === TabHeader=%s, MapImage=%s (vis=%d), InvContent=%d, TabButtonRow children=%d"),
		TabHeaderPanel ? *TabHeaderPanel->GetName() : TEXT("NULL"),
		MoonvilleMapImage ? *MoonvilleMapImage->GetName() : TEXT("NULL"),
		MoonvilleMapImage ? (int32)MoonvilleMapImage->GetVisibility() : -1,
		InventoryContentWidgets.Num(),
		TabButtonRow ? TabButtonRow->GetChildrenCount() : 0);
}

// Copy a template HorizontalBoxSlot's layout (padding/alignment/size) onto a live one, so the
// glyph/label sit inside our entries exactly as they sit inside the pack's footer button.
static void ZP_CopyHBoxSlot(const UWidget* TemplateWidget, UWidget* LiveWidget)
{
	const UHorizontalBoxSlot* Src = TemplateWidget ? Cast<UHorizontalBoxSlot>(TemplateWidget->Slot) : nullptr;
	UHorizontalBoxSlot* Dst = LiveWidget ? Cast<UHorizontalBoxSlot>(LiveWidget->Slot) : nullptr;
	if (!Src || !Dst)
	{
		if (Dst) { Dst->SetVerticalAlignment(VAlign_Center); }
		return;
	}
	Dst->SetPadding(Src->GetPadding());
	Dst->SetHorizontalAlignment(Src->GetHorizontalAlignment());
	Dst->SetVerticalAlignment(Src->GetVerticalAlignment());
	Dst->SetSize(Src->GetSize());
}

void UZP_InventoryTabWidget::BuildMapLegend()
{
	if (!MapLegendBox)
	{
		return;
	}

	// The bar's own entry widget supplies the templates: WBP_FooterButton2_Horror =
	// HorizontalBox [ InputActionWidget (CommonActionWidget) + Text_ActionName (CommonTextBlock) ].
	// Archetype-copying those two widgets clones every style property — font, colour, icon rim,
	// sizes — so an entry here is pixel-identical to an entry in the inventory's action bar.
	UClass* EntryCls = LoadClass<UUserWidget>(nullptr, *AZP_LegendEntryClassPath.ToString());
	UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(EntryCls);
	UDataTable* Actions = Cast<UDataTable>(AZP_LegendActionsTablePath.TryLoad());
	if (!WBGC || !Actions)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] Map legend: entry class (%s) or actions table (%s) failed to load"),
			WBGC ? TEXT("ok") : *AZP_LegendEntryClassPath.ToString(),
			Actions ? TEXT("ok") : *AZP_LegendActionsTablePath.ToString());
		return;
	}
	UWidgetTree* TplTree = WBGC->GetWidgetTreeArchetype();
	UWidget* GlyphTpl = TplTree ? TplTree->FindWidget<UWidget>(TEXT("InputActionWidget")) : nullptr;
	UCommonTextBlock* TextTpl = TplTree ? TplTree->FindWidget<UCommonTextBlock>(TEXT("Text_ActionName")) : nullptr;
	if (!GlyphTpl || !TextTpl)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] Map legend: footer button templates missing (glyph=%d text=%d)"),
			GlyphTpl != nullptr, TextTpl != nullptr);
		return;
	}

	// Glyph BRUSHES come straight from the CommonInput controller data assets — the same art the
	// inventory bar draws — resolved HERE, at build time, in C++. Nothing is left to CommonUI's
	// runtime resolution (CommonActionWidget stayed blank however it was fed; see checkpoint
	// 2026-08-06l/m). Device swap = UZP_GlyphDeviceSubsystem, the project's proven tracker.
	auto LoadControllerData = [](const FSoftClassPath& Path) -> const UCommonInputBaseControllerData*
	{
		if (UClass* Cls = LoadClass<UCommonInputBaseControllerData>(nullptr, *Path.ToString()))
		{
			return Cls->GetDefaultObject<UCommonInputBaseControllerData>();
		}
		return nullptr;
	};
	const UCommonInputBaseControllerData* KBMData = LoadControllerData(AZP_LegendKBMControllerDataClassPath);
	const UCommonInputBaseControllerData* PadData = LoadControllerData(AZP_LegendPadControllerDataClassPath);
	if (!KBMData || !PadData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] Map legend: controller data missing (KBM=%d pad=%d)"),
			KBMData != nullptr, PadData != nullptr);
	}

	// Key extraction from the row struct via reflection (KeyboardInputTypeInfo.Key /
	// DefaultGamepadInputTypeInfo.Key) — no CommonUI row-struct header needed.
	const UScriptStruct* KeyRowStruct = Actions->GetRowStruct();
	FStructProperty* KbmInfoProp = KeyRowStruct
		? CastField<FStructProperty>(KeyRowStruct->FindPropertyByName(TEXT("KeyboardInputTypeInfo"))) : nullptr;
	FStructProperty* PadInfoProp = KeyRowStruct
		? CastField<FStructProperty>(KeyRowStruct->FindPropertyByName(TEXT("DefaultGamepadInputTypeInfo"))) : nullptr;
	auto GetKeyFromInfo = [](FStructProperty* InfoProp, uint8* RowMem) -> FKey
	{
		if (!InfoProp || !RowMem)
		{
			return FKey();
		}
		uint8* InfoMem = InfoProp->ContainerPtrToValuePtr<uint8>(RowMem);
		FStructProperty* KeyProp = CastField<FStructProperty>(InfoProp->Struct->FindPropertyByName(TEXT("Key")));
		if (!KeyProp || KeyProp->Struct != TBaseStructure<FKey>::Get())
		{
			return FKey();
		}
		return *KeyProp->ContainerPtrToValuePtr<FKey>(InfoMem);
	};

	// Entry spacing: read off the live bar (10px on the horror bar) so a restyle follows through.
	FVector2D EntrySpacing(10.0f, 0.0f);
	if (UWidget* Bar = MoonvilleWidget ? MoonvilleWidget->GetWidgetFromName(TEXT("CommonBoundActionBar")) : nullptr)
	{
		if (FStructProperty* SP = CastField<FStructProperty>(Bar->GetClass()->FindPropertyByName(TEXT("EntrySpacing"))))
		{
			if (SP->Struct == TBaseStructure<FVector2D>::Get())
			{
				EntrySpacing = *SP->ContainerPtrToValuePtr<FVector2D>(Bar);
			}
		}
	}

	// Row labels come from the table's DisplayName — read via reflection so this file does not
	// need the CommonUI row-struct header.
	const UScriptStruct* RowStruct = Actions->GetRowStruct();
	FTextProperty* DisplayNameProp = RowStruct
		? CastField<FTextProperty>(RowStruct->FindPropertyByName(TEXT("DisplayName"))) : nullptr;

	LegendGlyphImages.Reset();
	MapLegendBox->ClearChildren();
	bool bFirst = true;
	for (const FString& EntryDef : AZP_LegendEntries)
	{
		TArray<FString> RowNames;
		EntryDef.ParseIntoArray(RowNames, TEXT(","));

		UHorizontalBox* Entry = NewObject<UHorizontalBox>(this);
		FText Label;
		int32 GlyphsAdded = 0;

		for (int32 i = 0; i < RowNames.Num(); ++i)
		{
			const FName RowName(*RowNames[i].TrimStartAndEnd());
			uint8* RowMem = Actions->FindRowUnchecked(RowName);
			if (!RowMem)
			{
				UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] Map legend: row '%s' not in %s"),
					*RowName.ToString(), *Actions->GetName());
				continue;
			}
			if (Label.IsEmpty() && DisplayNameProp)
			{
				Label = DisplayNameProp->GetPropertyValue_InContainer(RowMem);
			}

			// One image per row, both device brushes resolved NOW.
			FZPLegendGlyphImage GlyphImg;
			const FKey KBMKey = GetKeyFromInfo(KbmInfoProp, RowMem);
			const FKey PadKey = GetKeyFromInfo(PadInfoProp, RowMem);
			GlyphImg.bHasKBM = KBMData && KBMKey.IsValid() && KBMData->TryGetInputBrush(GlyphImg.KBMBrush, KBMKey);
			GlyphImg.bHasPad = PadData && PadKey.IsValid() && PadData->TryGetInputBrush(GlyphImg.PadBrush, PadKey);
			if (!GlyphImg.bHasKBM && !GlyphImg.bHasPad)
			{
				UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] Map legend: no brush for row '%s' (kbm=%s pad=%s)"),
					*RowName.ToString(), *KBMKey.ToString(), *PadKey.ToString());
				continue;
			}

			UImage* Img = NewObject<UImage>(this);
			Entry->AddChild(Img);
			ZP_CopyHBoxSlot(GlyphTpl, Img);
			GlyphImg.Image = Img;
			LegendGlyphImages.Add(GlyphImg);
			++GlyphsAdded;

			UE_LOG(LogTemp, Log, TEXT("[INVTAB-WIRE] Map legend glyph '%s': kbm=%s(%d) pad=%s(%d)"),
				*RowName.ToString(), *KBMKey.ToString(), GlyphImg.bHasKBM, *PadKey.ToString(), GlyphImg.bHasPad);
		}

		if (GlyphsAdded == 0)
		{
			Entry->ConditionalBeginDestroy();
			continue;
		}

		// Archetype copy of the bar's label — identical style/font/colour.
		UCommonTextBlock* Text = NewObject<UCommonTextBlock>(this, TextTpl->GetClass(), NAME_None, RF_NoFlags, TextTpl);
		Entry->AddChild(Text);
		ZP_CopyHBoxSlot(TextTpl, Text);
		Text->SetText(Label);

		MapLegendBox->AddChild(Entry);
		if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(Entry->Slot))
		{
			HSlot->SetPadding(FMargin(bFirst ? 0.0f : EntrySpacing.X, 0.0f, 0.0f, 0.0f));
			HSlot->SetVerticalAlignment(VAlign_Center);
		}
		bFirst = false;
	}

	// Stamp the current device's brushes immediately — brushes are plain data on plain UImages,
	// no construction-order dependency.
	ApplyLegendDeviceBrushes(/*bForce*/ true);
}

void UZP_InventoryTabWidget::ApplyLegendDeviceBrushes(bool bForce)
{
	bool bGamepad = false;
	if (UWorld* World = GetWorld())
	{
		if (UZP_GlyphDeviceSubsystem* Glyphs = World->GetSubsystem<UZP_GlyphDeviceSubsystem>())
		{
			bGamepad = Glyphs->IsGamepadActive();
		}
	}
	if (!bForce && bGamepad == bLegendGlyphsGamepad)
	{
		return;
	}
	bLegendGlyphsGamepad = bGamepad;

	for (FZPLegendGlyphImage& GlyphImg : LegendGlyphImages)
	{
		UImage* Img = GlyphImg.Image.Get();
		if (!Img)
		{
			continue;
		}
		const bool bHas = bGamepad ? GlyphImg.bHasPad : GlyphImg.bHasKBM;
		if (bHas)
		{
			Img->SetBrush(bGamepad ? GlyphImg.PadBrush : GlyphImg.KBMBrush);
			Img->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// No art for this key on this device (e.g. LT's wheel-down counterpart) — hide rather
			// than draw a blank box.
			Img->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

UWidget* UZP_InventoryTabWidget::FindMoonvilleWidgetRef(const FName& PropertyName) const
{
	if (!MoonvilleWidget) return nullptr;

	UClass* WidgetClass = MoonvilleWidget->GetClass();
	FObjectProperty* Prop = CastField<FObjectProperty>(WidgetClass->FindPropertyByName(PropertyName));
	if (Prop)
	{
		UObject* Obj = Prop->GetObjectPropertyValue(Prop->ContainerPtrToValuePtr<void>(MoonvilleWidget));
		return Cast<UWidget>(Obj);
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::BindToCharacter(AZP_GraceCharacter* Character)
{
	if (!Character) return;

	CachedCharacter = Character;
	CachedMapComp = Character->MapComp;
	CachedNoteComp = Character->NoteComp;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InventoryTabWidget: Bound to %s (MapComp=%s, NoteComp=%s)"),
		*Character->GetName(),
		CachedMapComp.IsValid() ? TEXT("valid") : TEXT("NULL"),
		CachedNoteComp.IsValid() ? TEXT("valid") : TEXT("NULL"));
}

// ---------------------------------------------------------------------------
// NotifyMoonvilleToggled — hint from input handler that Moonville was just toggled
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::NotifyMoonvilleToggled(EZP_InventoryTab DesiredTab)
{
	PendingTab = DesiredTab;
	bSearchForWidget = true;

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] InventoryTabWidget: NotifyMoonvilleToggled (tab=%d)"),
		(int32)DesiredTab);
}

// ---------------------------------------------------------------------------
// Tab Cycling
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::CycleTab(int32 Direction)
{
	if (!bIsOpen || !bTabsWired) return;

	constexpr int32 TabCount = 3; // Map, Inventory, Notes
	int32 Current = static_cast<int32>(CurrentTab);
	int32 Next = ((Current + Direction) % TabCount + TabCount) % TabCount; // Wraps both directions
	SwitchToTab(static_cast<EZP_InventoryTab>(Next));
}

// ---------------------------------------------------------------------------
// Tab Switching
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::SwitchToTab(EZP_InventoryTab Tab)
{
	CurrentTab = Tab;
	UpdateTabButtonStyles();

	switch (Tab)
	{
	case EZP_InventoryTab::Map:
		SetInventoryContentVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		// Grunge behind the map; legend visibility is decided by RefreshMapDisplay (only
		// meaningful when a map is actually shown — no controls without a map).
		if (TabBackdropImage) TabBackdropImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		// Fresh view every time the tab is opened — fitted and centred, following the player.
		ViewedAreaID = NAME_None;
		ResetMapView();
		UpdateMapLayout();
		RefreshMapDisplay();
		break;

	case EZP_InventoryTab::Inventory:
		SetInventoryContentVisibility(ESlateVisibility::Visible);
		if (MapViewport) MapViewport->SetVisibility(ESlateVisibility::Collapsed);
		if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
		if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
		if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		// Inventory has Moonville's own grunge inside InventoryCanvas — ours would double it.
		if (TabBackdropImage) TabBackdropImage->SetVisibility(ESlateVisibility::Collapsed);
		if (MapLegendBox) MapLegendBox->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case EZP_InventoryTab::Notes:
		SetInventoryContentVisibility(ESlateVisibility::Collapsed);
		if (MapViewport) MapViewport->SetVisibility(ESlateVisibility::Collapsed);
		if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabBackdropImage) TabBackdropImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (MapLegendBox) MapLegendBox->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
		if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
		// Scan inventory for notes on-demand (delegate may not have fired)
		if (CachedCharacter.IsValid())
		{
			CachedCharacter->ScanInventoryForNotes();
		}
		if (NotesWidget)
		{
			NotesWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			NotesWidget->RefreshNoteList();
		}
		break;
	}
}

void UZP_InventoryTabWidget::SetInventoryContentVisibility(ESlateVisibility InVisibility)
{
	for (int32 i = 0; i < InventoryContentWidgets.Num(); ++i)
	{
		UWidget* W = InventoryContentWidgets[i];
		if (!W)
		{
			continue;
		}
		// "Show" = restore what Moonville authored (captured at wiring) so
		// hidden modals stay hidden and hit-testing matches their design.
		if (InVisibility == ESlateVisibility::Visible && InventoryContentSavedVis.IsValidIndex(i))
		{
			W->SetVisibility(InventoryContentSavedVis[i]);
		}
		else
		{
			W->SetVisibility(InVisibility);
		}
	}
}

void UZP_InventoryTabWidget::UpdateTabButtonStyles()
{
	auto SetTabActive = [this](UButton* Button, bool bActive)
	{
		if (!Button || Button->GetChildrenCount() == 0) return;
		if (UTextBlock* Text = Cast<UTextBlock>(Button->GetChildAt(0)))
		{
			Text->SetColorAndOpacity(FSlateColor(bActive ? AZP_ActiveTabColor : AZP_InactiveTabColor));
		}
	};

	SetTabActive(MapTabButton,       CurrentTab == EZP_InventoryTab::Map);
	SetTabActive(InventoryTabButton, CurrentTab == EZP_InventoryTab::Inventory);
	SetTabActive(NotesTabButton,     CurrentTab == EZP_InventoryTab::Notes);
}

void UZP_InventoryTabWidget::OnMapTabClicked()
{
	SwitchToTab(EZP_InventoryTab::Map);
}

void UZP_InventoryTabWidget::OnInventoryTabClicked()
{
	SwitchToTab(EZP_InventoryTab::Inventory);
}

void UZP_InventoryTabWidget::OnNotesTabClicked()
{
	SwitchToTab(EZP_InventoryTab::Notes);
}

// ---------------------------------------------------------------------------
// Map Display
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::RefreshMapDisplay()
{
	// Moonville's own MapImage is never the map surface any more — MapSurface inside MapViewport
	// is. Keep it collapsed so it cannot draw over us.
	if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);

	auto ShowNothing = [this](const FText& AreaText, const FText& Message)
	{
		if (MapViewport)     MapViewport->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (MapLegendBox)    MapLegendBox->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText) TabAreaNameText->SetText(AreaText);
		if (TabNoMapText)
		{
			TabNoMapText->SetText(Message);
			TabNoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	};

	if (!CachedMapComp.IsValid())
	{
		ShowNothing(AZP_UnknownAreaText, AZP_NoMapAvailableText);
		CachedVolume = nullptr;
		return;
	}

	// Which floor are we LOOKING at? ViewedAreaID is set by floor cycling; None = follow the
	// player. A stale ViewedAreaID (area no longer resolvable) silently falls back to following.
	const FName PlayerAreaID = CachedMapComp->GetCurrentAreaID();
	FName AreaID = ViewedAreaID.IsNone() ? PlayerAreaID : ViewedAreaID;
	AZP_MapVolume* Volume = AreaID.IsNone() ? nullptr : CachedMapComp->GetAreaVolume(AreaID);
	if (!Volume && !ViewedAreaID.IsNone())
	{
		ViewedAreaID = NAME_None;
		AreaID = PlayerAreaID;
		Volume = CachedMapComp->GetCurrentVolume();
	}

	bViewingPlayerArea = (!AreaID.IsNone() && AreaID == PlayerAreaID);

	if (AreaID.IsNone() || !Volume)
	{
		ShowNothing(AZP_UnknownAreaText, AZP_NoMapAvailableText);
		CachedVolume = nullptr;
		return;
	}

	CachedVolume = Volume;

	if (!CachedMapComp->IsMapDiscovered(AreaID))
	{
		ShowNothing(Volume->AZP_AreaDisplayName, AZP_MapNotFoundText);
		return;
	}

	// Show discovered map
	if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
	if (TabAreaNameText) TabAreaNameText->SetText(Volume->AZP_AreaDisplayName);

	if (MapSurface && MapViewport && Volume->AZP_MapTexture)
	{
		MapSurface->SetBrushFromTexture(Volume->AZP_MapTexture);
		MapSurface->SetVisibility(ESlateVisibility::HitTestInvisible);
		MapViewport->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (MapLegendBox) MapLegendBox->SetVisibility(ESlateVisibility::HitTestInvisible);

		// The marker only means anything on the floor the player is actually standing on.
		if (TabPlayerMarker)
		{
			TabPlayerMarker->SetVisibility(bViewingPlayerArea
				? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	else
	{
		ShowNothing(Volume->AZP_AreaDisplayName, AZP_NoMapAvailableText);
	}
}

// ---------------------------------------------------------------------------
// Notes (placeholder — full implementation deferred)
// ---------------------------------------------------------------------------

void UZP_InventoryTabWidget::SelectNote(int32 NoteIndex)
{
	SelectedNoteIndex = NoteIndex;

	if (NotesWidget)
	{
		NotesWidget->SelectNote(NoteIndex);
	}
}

// ---------------------------------------------------------------------------
// Map view — layout, zoom, pan, floor cycling
// ---------------------------------------------------------------------------

bool UZP_InventoryTabWidget::IsMapInputActive() const
{
	return bIsOpen && bTabsWired && CurrentTab == EZP_InventoryTab::Map && MapViewport && MapSurface;
}

bool UZP_InventoryTabWidget::HandleMapKey(const FKey& Key)
{
	if (!IsMapInputActive())
	{
		return false;
	}
	if (Key == AZP_MapFloorUpKey || (AZP_MapFloorUpGamepadKey.IsValid() && Key == AZP_MapFloorUpGamepadKey))
	{
		CycleMapFloor(1);
		return true;
	}
	if (Key == AZP_MapFloorDownKey || (AZP_MapFloorDownGamepadKey.IsValid() && Key == AZP_MapFloorDownGamepadKey))
	{
		CycleMapFloor(-1);
		return true;
	}
	return false;
}

bool UZP_InventoryTabWidget::IsNotesInputActive() const
{
	return bIsOpen && bTabsWired && CurrentTab == EZP_InventoryTab::Notes && NotesWidget;
}

bool UZP_InventoryTabWidget::HandleNotesKey(const FKey& Key)
{
	if (!IsNotesInputActive())
	{
		return false;
	}
	// D-pad = scroll the note CONTENT (right panel). List SELECTION is the left stick,
	// handled in NativeTick (analog).
	if (AZP_NotesScrollUpGamepadKey.IsValid() && Key == AZP_NotesScrollUpGamepadKey)
	{
		NotesWidget->ScrollContent(-AZP_NotesContentScrollStep);
		return true;
	}
	if (AZP_NotesScrollDownGamepadKey.IsValid() && Key == AZP_NotesScrollDownGamepadKey)
	{
		NotesWidget->ScrollContent(AZP_NotesContentScrollStep);
		return true;
	}
	return false;
}

void UZP_InventoryTabWidget::GetCycleableAreas(TArray<FName>& OutAreas) const
{
	OutAreas.Reset();
	if (!CachedMapComp.IsValid())
	{
		return;
	}
	for (const FName& AreaID : CachedMapComp->DiscoveredMaps)
	{
		if (!AreaID.IsNone() && CachedMapComp->GetAreaVolume(AreaID))
		{
			OutAreas.Add(AreaID);
		}
	}
	// TSet iteration order is not stable across runs. Real floor ordering lands in its own pass
	// (dev: "order of floors will come in another session"); sort by ID so cycling is at least
	// repeatable in the meantime.
	OutAreas.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });
}

void UZP_InventoryTabWidget::CycleMapFloor(int32 Direction)
{
	TArray<FName> Areas;
	GetCycleableAreas(Areas);
	if (Areas.Num() <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MAP-TAB] Floor cycle ignored — %d discovered area(s)"), Areas.Num());
		return;
	}

	const FName Current = !ViewedAreaID.IsNone()
		? ViewedAreaID
		: (CachedMapComp.IsValid() ? CachedMapComp->GetCurrentAreaID() : NAME_None);

	int32 Index = Areas.IndexOfByKey(Current);
	if (Index == INDEX_NONE)
	{
		Index = 0;
	}
	const int32 Next = ((Index + Direction) % Areas.Num() + Areas.Num()) % Areas.Num();
	ViewedAreaID = Areas[Next];

	UE_LOG(LogTemp, Warning, TEXT("[MAP-TAB] Floor %s -> %s (%d/%d)"),
		*Current.ToString(), *ViewedAreaID.ToString(), Next + 1, Areas.Num());

	ResetMapView();
	RefreshMapDisplay();
}

void UZP_InventoryTabWidget::ResetMapView()
{
	MapZoom = FMath::Max(AZP_MapZoomMin, 1.0f);
	MapPan = FVector2D::ZeroVector;
}

void UZP_InventoryTabWidget::UpdateMapLayout()
{
	if (!MapViewport || !MoonvilleMapImage)
	{
		return;
	}
	UPanelWidget* MapParent = MoonvilleMapImage->GetParent();
	UCanvasPanelSlot* VSlot = Cast<UCanvasPanelSlot>(MapViewport->Slot);
	if (!MapParent || !VSlot)
	{
		return;
	}

	// Measure the real header band rather than trusting a constant: convert the bottom edge of
	// TabHeader into the map canvas' local space. Falls back to AZP_MapHeaderHeight until Slate
	// has cached a geometry for it (first frame after open).
	float HeaderBottom = AZP_MapHeaderHeight;
	if (TabHeaderPanel)
	{
		const FGeometry& HeaderGeo = TabHeaderPanel->GetCachedGeometry();
		const FGeometry& ParentGeo = MapParent->GetCachedGeometry();
		const FVector2D HeaderSize = HeaderGeo.GetLocalSize();
		if (HeaderSize.Y > 1.0f && ParentGeo.GetLocalSize().Y > 1.0f)
		{
			const FVector2D AbsBottomLeft = HeaderGeo.LocalToAbsolute(FVector2D(0.0f, HeaderSize.Y));
			const FVector2D LocalBottom = ParentGeo.AbsoluteToLocal(AbsBottomLeft);
			HeaderBottom = FMath::Max(0.0f, LocalBottom.Y);
		}
	}

	const float M = AZP_MapViewportMargin;
	VSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	// Canvas offsets with 0-1 anchors are (left, top, right, bottom) insets.
	VSlot->SetOffsets(FMargin(M, HeaderBottom + M, M, M));
}

void UZP_InventoryTabWidget::ApplyMapView()
{
	if (!MapViewport || !MapSurface)
	{
		return;
	}

	const FVector2D ViewSize = MapViewport->GetCachedGeometry().GetLocalSize();
	if (ViewSize.X < 1.0f || ViewSize.Y < 1.0f)
	{
		return;
	}
	MapViewportSize = ViewSize;

	// Fit the map inside the viewport preserving aspect (letterbox, never stretch).
	FVector2D TexSize(1024.0f, 1024.0f);
	if (const UTexture2D* Tex = Cast<UTexture2D>(MapSurface->GetBrush().GetResourceObject()))
	{
		const float TW = static_cast<float>(Tex->GetSizeX());
		const float TH = static_cast<float>(Tex->GetSizeY());
		if (TW > 0.0f && TH > 0.0f)
		{
			TexSize = FVector2D(TW, TH);
		}
	}
	const float FitScale = FMath::Min(ViewSize.X / TexSize.X, ViewSize.Y / TexSize.Y);
	MapFittedSize = TexSize * FitScale;

	MapZoom = FMath::Clamp(MapZoom, AZP_MapZoomMin, AZP_MapZoomMax);
	const FVector2D DrawSize = MapFittedSize * MapZoom;

	// Clamp the pan so the map can never be dragged off the viewport. When the map is smaller
	// than the viewport on an axis it is simply centred on that axis.
	auto ClampAxis = [](float PanV, float Draw, float View) -> float
	{
		if (Draw <= View)
		{
			return (View - Draw) * 0.5f;
		}
		return FMath::Clamp(PanV, View - Draw, 0.0f);
	};
	MapPan.X = ClampAxis(MapPan.X, DrawSize.X, ViewSize.X);
	MapPan.Y = ClampAxis(MapPan.Y, DrawSize.Y, ViewSize.Y);

	if (UCanvasPanelSlot* SSlot = Cast<UCanvasPanelSlot>(MapSurface->Slot))
	{
		SSlot->SetPosition(MapPan);
		SSlot->SetSize(DrawSize);
	}

	// Marker rides the same transform as the map.
	if (TabPlayerMarker && TabPlayerMarker->GetVisibility() != ESlateVisibility::Collapsed)
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC && PC->GetPawn())
		{
			const FVector2D UV = WorldToMapUV(PC->GetPawn()->GetActorLocation());
			const FVector2D MarkerPos = MapPan + UV * DrawSize - (AZP_TabMarkerSize * 0.5f);
			if (UCanvasPanelSlot* MSlot = Cast<UCanvasPanelSlot>(TabPlayerMarker->Slot))
			{
				MSlot->SetPosition(MarkerPos);
				MSlot->SetSize(AZP_TabMarkerSize);
			}
			TabPlayerMarker->SetRenderTransformAngle(-PC->GetControlRotation().Yaw - 90.0f);
		}
	}
}

// ---------------------------------------------------------------------------
// Coordinate Conversion
// ---------------------------------------------------------------------------

FVector2D UZP_InventoryTabWidget::WorldToMapUV(const FVector& WorldLocation) const
{
	if (!CachedVolume.IsValid()) return FVector2D(0.5f, 0.5f);

	const FVector2D WorldMin = CachedVolume->GetWorldBoundsMin();
	const FVector2D WorldMax = CachedVolume->GetWorldBoundsMax();
	const FVector2D WorldSize = WorldMax - WorldMin;
	if (WorldSize.X < 1.0f || WorldSize.Y < 1.0f) return FVector2D(0.5f, 0.5f);

	FVector2D UV;
	UV.X = FMath::Clamp((WorldLocation.X - WorldMin.X) / WorldSize.X, 0.0f, 1.0f);
	UV.Y = FMath::Clamp((WorldLocation.Y - WorldMin.Y) / WorldSize.Y, 0.0f, 1.0f);
	// No Y flip — PNG export already renders Y-down (screen convention)
	return UV;
}
