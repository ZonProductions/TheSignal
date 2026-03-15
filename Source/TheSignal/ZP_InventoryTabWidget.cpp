// Copyright The Signal. All Rights Reserved.

#include "ZP_InventoryTabWidget.h"
#include "ZP_MapComponent.h"
#include "ZP_NoteComponent.h"
#include "ZP_NotesWidget.h"
#include "ZP_MapVolume.h"
#include "ZP_GraceCharacter.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
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
	if (!InventoryWidgetClass)
	{
		InventoryWidgetClass = LoadClass<UUserWidget>(nullptr,
			TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror.WBP_InventoryMenu_Horror_C"));
	}

	// Auto-load Notes widget class
	if (!NotesWidgetClass)
	{
		NotesWidgetClass = LoadClass<UZP_NotesWidget>(nullptr,
			TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes_C"));
	}

	// Non-visual controller but needs to receive key events for tab cycling.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	bIsOpen = false;
}

FReply UZP_InventoryTabWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] NativeOnKeyDown: Key=%s bIsOpen=%d bTabsWired=%d HasFocus=%d"),
		*Key.ToString(), bIsOpen, bTabsWired, HasKeyboardFocus());

	if (!bIsOpen || !bTabsWired)
	{
		return FReply::Unhandled();
	}

	if (Key == EKeys::Q)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Cycling LEFT"));
		CycleTab(-1);
		return FReply::Handled();
	}
	else if (Key == EKeys::E)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-KEY] Cycling RIGHT"));
		CycleTab(1);
		return FReply::Handled();
	}

	return FReply::Unhandled();
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
			if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
			if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
			if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
			if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
			if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		}

		bIsOpen = false;
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
			if (PC->WasInputKeyJustPressed(EKeys::Q))
			{
				CycleTab(-1);
			}
			else if (PC->WasInputKeyJustPressed(EKeys::E))
			{
				CycleTab(1);
			}
		}
	}

	// --- Update map player marker ---
	if (CurrentTab == EZP_InventoryTab::Map && CachedMapComp.IsValid() && CachedVolume.IsValid())
	{
		APlayerController* PC = GetOwningPlayer();
		if (!PC || !PC->GetPawn()) return;

		if (TabPlayerMarker && MoonvilleMapImage)
		{
			const FGeometry MapGeo = MoonvilleMapImage->GetCachedGeometry();
			const FVector2D ImgSize = MapGeo.GetLocalSize();
			if (ImgSize.X < 1.0f || ImgSize.Y < 1.0f) return;

			const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
			FVector2D UV = WorldToMapUV(PlayerLoc);
			// DEBUG: override disabled — using real UV
			//UV = FVector2D(0.5f, 0.5f);

			// When showing map: reset image to fill the widget area
			MoonvilleMapImage->SetRenderScale(FVector2D(1.0f, 1.0f));
			MoonvilleMapImage->SetRenderTranslation(FVector2D::ZeroVector);
			if (UCanvasPanelSlot* ImgSlot = Cast<UCanvasPanelSlot>(MoonvilleMapImage->Slot))
			{
				ImgSlot->SetPosition(FVector2D::ZeroVector);
				ImgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				ImgSlot->SetOffsets(FMargin(0, 0, 0, 0));
			}

			// Marker at UV position on the image (image now starts at 0,0)
			const FVector2D MarkerPos = UV * ImgSize - (TabMarkerSize * 0.5f);

			// Use slot position if canvas, otherwise render translation
			if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(TabPlayerMarker->Slot))
			{
				MarkerSlot->SetPosition(MarkerPos);
				MarkerSlot->SetSize(TabMarkerSize);
			}
			else
			{
				TabPlayerMarker->SetRenderTranslation(MarkerPos);
			}
			TabPlayerMarker->SetRenderTransformAngle(-PC->GetControlRotation().Yaw - 90.0f);

			static int32 MapLogCounter = 0;
			if (MapLogCounter++ % 60 == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MAP-TAB] UV=(%.3f,%.3f) ImgSize=(%.0f,%.0f) Marker=(%.0f,%.0f)"),
					UV.X, UV.Y, ImgSize.X, ImgSize.Y, MarkerPos.X, MarkerPos.Y);
			}
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
	Text->SetColorAndOpacity(FSlateColor(InactiveTabColor));
	Text->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 14;
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
	if (!InventoryWidgetClass || !GetWorld()) return;

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Found, InventoryWidgetClass, false);

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
	if (TabPlayerMarker) { TabPlayerMarker->RemoveFromParent(); TabPlayerMarker = nullptr; }
	if (TabAreaNameText) { TabAreaNameText->RemoveFromParent(); TabAreaNameText = nullptr; }
	if (TabNoMapText) { TabNoMapText->RemoveFromParent(); TabNoMapText = nullptr; }
	if (NotesWidget) { NotesWidget->RemoveFromParent(); NotesWidget = nullptr; }

	InventoryContentWidgets.Empty();
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

	// --- Create a HorizontalBox for tab buttons (TabHeader is a VerticalBox) ---
	TabButtonRow = NewObject<UHorizontalBox>(this);
	TabHeaderPanel->AddChild(TabButtonRow);

	MapTabButton = CreateTabButton(TEXT("MAP"));
	TabButtonRow->AddChild(MapTabButton);
	MapTabButton->OnClicked.AddDynamic(this, &UZP_InventoryTabWidget::OnMapTabClicked);

	InventoryTabButton = CreateTabButton(TEXT("INVENTORY"));
	TabButtonRow->AddChild(InventoryTabButton);
	InventoryTabButton->OnClicked.AddDynamic(this, &UZP_InventoryTabWidget::OnInventoryTabClicked);

	NotesTabButton = CreateTabButton(TEXT("NOTES"));
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

	// --- Create player marker for map ---
	if (MoonvilleMapImage)
	{
		UPanelWidget* MapParent = MoonvilleMapImage->GetParent();
		if (MapParent)
		{
			// Player marker — green triangle (arrow) that rotates with player direction
			TabPlayerMarker = NewObject<UImage>(this);
			TabPlayerMarker->SetColorAndOpacity(FLinearColor(0.0f, 1.0f, 0.3f, 1.0f));
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
			MapParent->AddChild(TabPlayerMarker);

			if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(TabPlayerMarker->Slot))
			{
				CSlot->SetSize(TabMarkerSize);
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
				Font.Size = 20;
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
			TabNoMapText->SetText(FText::FromString(TEXT("No map available")));
			TabNoMapText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
			{
				FSlateFontInfo Font = TabNoMapText->GetFont();
				Font.Size = 18;
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
	if (NotesWidgetClass && MoonvilleMapImage && MoonvilleMapImage->GetParent())
	{
		NotesWidget = CreateWidget<UZP_NotesWidget>(this, NotesWidgetClass);
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
	else if (!NotesWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[INVTAB-WIRE] NotesWidgetClass is null — notes tab will be empty"));
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
		if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		RefreshMapDisplay();
		break;

	case EZP_InventoryTab::Inventory:
		SetInventoryContentVisibility(ESlateVisibility::Visible);
		if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText) TabAreaNameText->SetVisibility(ESlateVisibility::Collapsed);
		if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
		if (NotesWidget) NotesWidget->SetVisibility(ESlateVisibility::Collapsed);
		break;

	case EZP_InventoryTab::Notes:
		SetInventoryContentVisibility(ESlateVisibility::Collapsed);
		if (MoonvilleMapImage) MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
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
	for (UWidget* W : InventoryContentWidgets)
	{
		if (W)
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
			Text->SetColorAndOpacity(FSlateColor(bActive ? ActiveTabColor : InactiveTabColor));
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
	if (!CachedMapComp.IsValid())
	{
		if (MoonvilleMapImage)  MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker)   TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText)   TabAreaNameText->SetText(FText::FromString(TEXT("Unknown Area")));
		if (TabNoMapText)
		{
			TabNoMapText->SetText(FText::FromString(TEXT("No map available")));
			TabNoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		CachedVolume = nullptr;
		return;
	}

	const FName AreaID = CachedMapComp->GetCurrentAreaID();
	AZP_MapVolume* Volume = CachedMapComp->GetCurrentVolume();

	if (AreaID.IsNone() || !Volume)
	{
		if (MoonvilleMapImage)  MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker)   TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText)   TabAreaNameText->SetText(FText::FromString(TEXT("Unknown Area")));
		if (TabNoMapText)
		{
			TabNoMapText->SetText(FText::FromString(TEXT("No map available")));
			TabNoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		CachedVolume = nullptr;
		return;
	}

	CachedVolume = Volume;

	if (!CachedMapComp->IsMapDiscovered(AreaID))
	{
		if (MoonvilleMapImage)  MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
		if (TabPlayerMarker)   TabPlayerMarker->SetVisibility(ESlateVisibility::Collapsed);
		if (TabAreaNameText)   TabAreaNameText->SetText(Volume->AreaDisplayName);
		if (TabNoMapText)
		{
			TabNoMapText->SetText(FText::FromString(TEXT("Map not found yet")));
			TabNoMapText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	// Show discovered map
	if (TabNoMapText) TabNoMapText->SetVisibility(ESlateVisibility::Collapsed);
	if (TabAreaNameText) TabAreaNameText->SetText(Volume->AreaDisplayName);

	if (MoonvilleMapImage && Volume->MapTexture)
	{
		MoonvilleMapImage->SetBrushFromTexture(Volume->MapTexture);
		MoonvilleMapImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (MoonvilleMapImage)
	{
		MoonvilleMapImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TabPlayerMarker) TabPlayerMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
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
