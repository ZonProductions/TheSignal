// Copyright The Signal. All Rights Reserved.

#if WITH_EDITOR

#include "ZP_EditorWidgetUtils.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#include "ZP_NotesWidget.h"
#include "ZP_NoteEntryWidget.h"

bool UZP_EditorWidgetUtils::AddTabsToInventoryMenu()
{
	// Load the widget blueprint
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror.WBP_InventoryMenu_Horror"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] AddTabsToInventoryMenu: Failed to load WBP_InventoryMenu_Horror"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] AddTabsToInventoryMenu: WidgetTree is null"));
		return false;
	}

	// Check if tabs already added (prevent double-run)
	if (Tree->FindWidget(TEXT("TabContentSwitcher")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] AddTabsToInventoryMenu: Tabs already exist — skipping"));
		return true;
	}

	UWidget* OldRoot = Tree->RootWidget;
	if (!OldRoot)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] AddTabsToInventoryMenu: No existing root widget"));
		return false;
	}

	WBP->Modify();

	// ---------------------------------------------------------------
	// Build new structure:
	//
	// NewRoot (CanvasPanel, anchored full-screen like old root)
	// └── MainVBox (VerticalBox, fills canvas)
	//     ├── TabBar (HorizontalBox, auto-height)
	//     │   ├── MapTabButton (Button > TextBlock "MAP")
	//     │   ├── InventoryTabButton (Button > TextBlock "INVENTORY")
	//     │   └── NotesTabButton (Button > TextBlock "NOTES")
	//     └── TabContentSwitcher (WidgetSwitcher, fills remaining)
	//         ├── [0] MapTabPanel (CanvasPanel — empty, for map content)
	//         ├── [1] OldRoot (existing inventory UI — UNCHANGED)
	//         └── [2] NotesTabPanel (CanvasPanel — empty, for notes)
	// ---------------------------------------------------------------

	// New root canvas (same type as typical WBP root)
	UCanvasPanel* NewRoot = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TabRootCanvas"));

	// Main vertical layout filling the canvas
	UVerticalBox* MainVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TabMainVBox"));
	NewRoot->AddChild(MainVBox);
	if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(MainVBox->Slot))
	{
		CSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CSlot->SetOffsets(FMargin(0.0f));
	}

	// --- Tab bar ---
	UHorizontalBox* TabBar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabBar"));
	MainVBox->AddChild(TabBar);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(TabBar->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		VSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	// Helper lambda to create a tab button
	auto CreateTabBtn = [&](const FString& Name, const FString& Label) -> UButton*
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*Name));
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(Name + TEXT("_Text"))));
		Text->SetText(FText::FromString(Label));
		Btn->AddChild(Text);
		return Btn;
	};

	UButton* MapBtn = CreateTabBtn(TEXT("MapTabButton"), TEXT("MAP"));
	TabBar->AddChild(MapBtn);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(MapBtn->Slot))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(2.0f, 2.0f));
	}

	UButton* InvBtn = CreateTabBtn(TEXT("InventoryTabButton"), TEXT("INVENTORY"));
	TabBar->AddChild(InvBtn);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(InvBtn->Slot))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(2.0f, 2.0f));
	}

	UButton* NotesBtn = CreateTabBtn(TEXT("NotesTabButton"), TEXT("NOTES"));
	TabBar->AddChild(NotesBtn);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(NotesBtn->Slot))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetPadding(FMargin(2.0f, 2.0f));
	}

	// --- Content switcher ---
	UWidgetSwitcher* Switcher = Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("TabContentSwitcher"));
	MainVBox->AddChild(Switcher);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(Switcher->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// Index 0: Map panel (empty canvas for now — C++ populates at runtime)
	UCanvasPanel* MapPanel = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapTabPanel"));
	Switcher->AddChild(MapPanel);

	// Index 1: Existing inventory content (the old root, reparented)
	Tree->RootWidget = nullptr; // Detach old root
	Switcher->AddChild(OldRoot);

	// Index 2: Notes panel (empty canvas for now — C++ populates at runtime)
	UCanvasPanel* NotesPanel = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NotesTabPanel"));
	Switcher->AddChild(NotesPanel);

	// Set new root
	Tree->RootWidget = NewRoot;

	// Mark modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);
	WBP->GetPackage()->MarkPackageDirty();

	// Save
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		RF_Standalone,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()));

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] AddTabsToInventoryMenu: SUCCESS — tabs added to WBP_InventoryMenu_Horror (saved=%s)"),
		bSaved ? TEXT("true") : TEXT("false"));

	return true;
}

// ---------------------------------------------------------------------------
// SetupNotesWidget — add BindWidget widgets to WBP_Notes and reparent
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::SetupNotesWidget()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetupNotesWidget: Failed to load WBP_Notes"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetupNotesWidget: WidgetTree is null"));
		return false;
	}

	// Check if already set up
	if (Tree->FindWidget(TEXT("NoteListScrollBox")))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SetupNotesWidget: Already set up — skipping"));
		return true;
	}

	WBP->Modify();

	// Capture the existing root (BackgroundImage for EGUI styling)
	UWidget* OldRoot = Tree->RootWidget;

	// Build new structure:
	// Overlay (root)
	// ├── OldRoot (BackgroundImage — EGUI styling, fills overlay)
	// └── HorizontalBox (content)
	//     ├── SizeBox (left panel, width override)
	//     │   └── Overlay
	//     │       ├── ScrollBox "NoteListScrollBox"
	//     │       └── TextBlock "NotesEmptyText" (centered, shown when empty)
	//     └── VerticalBox (right panel, fills remaining)
	//         ├── TextBlock "NoteTitle" (auto height)
	//         └── TextBlock "NoteContent" (fills remaining)

	UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("NotesRoot"));

	// Re-add old root (BackgroundImage) as first child of overlay for styling
	if (OldRoot)
	{
		Tree->RootWidget = nullptr;
		Root->AddChild(OldRoot);
		if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(OldRoot->Slot))
		{
			OSlot->SetHorizontalAlignment(HAlign_Fill);
			OSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// Content HorizontalBox
	UHorizontalBox* ContentHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NotesContentHBox"));
	Root->AddChild(ContentHBox);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(ContentHBox->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// === LEFT PANEL (Column A) — 45% width, note list ===
	UOverlay* LeftOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LeftPanelOverlay"));
	ContentHBox->AddChild(LeftOverlay);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(LeftOverlay->Slot))
	{
		FSlateChildSize FillLeft(ESlateSizeRule::Fill);
		FillLeft.Value = 0.45f;
		HSlot->SetSize(FillLeft);
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(8.0f, 8.0f, 0.0f, 8.0f));
	}

	// NoteListScrollBox (BindWidget)
	UScrollBox* NoteListScrollBox = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NoteListScrollBox"));
	LeftOverlay->AddChild(NoteListScrollBox);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(NoteListScrollBox->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// NotesEmptyText (BindWidgetOptional)
	UTextBlock* NotesEmptyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotesEmptyText"));
	NotesEmptyText->SetText(FText::FromString(TEXT("No notes collected")));
	NotesEmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f, 0.5f)));
	{
		FSlateFontInfo Font = NotesEmptyText->GetFont();
		Font.Size = 16;
		NotesEmptyText->SetFont(Font);
	}
	NotesEmptyText->SetJustification(ETextJustify::Center);
	LeftOverlay->AddChild(NotesEmptyText);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(NotesEmptyText->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Center);
		OSlot->SetVerticalAlignment(VAlign_Center);
	}

	// === RIGHT PANEL (Column B) — 55% width, dark background ===
	UBorder* RightBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBorder"));
	ContentHBox->AddChild(RightBorder);
	RightBorder->SetBrushColor(FLinearColor(0.025f, 0.03f, 0.028f, 0.88f));
	RightBorder->SetPadding(FMargin(24.0f, 20.0f, 24.0f, 20.0f));
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(RightBorder->Slot))
	{
		FSlateChildSize FillRight(ESlateSizeRule::Fill);
		FillRight.Value = 0.55f;
		HSlot->SetSize(FillRight);
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(4.0f, 8.0f, 8.0f, 8.0f));
	}

	UVerticalBox* RightVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightPanelVBox"));
	RightBorder->AddChild(RightVBox);

	// NoteTitle (BindWidget) — left-aligned, 2x font, bold
	UTextBlock* NoteTitle = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoteTitle"));
	NoteTitle->SetText(FText::FromString(TEXT("")));
	NoteTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.93f, 0.93f, 1.0f)));
	{
		FSlateFontInfo Font = NoteTitle->GetFont();
		Font.Size = 30;
		Font.TypefaceFontName = FName("Bold");
		NoteTitle->SetFont(Font);
	}
	NoteTitle->SetJustification(ETextJustify::Left);
	RightVBox->AddChild(NoteTitle);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(NoteTitle->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		VSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
	}

	// Spacer above divider
	USizeBox* TitleSpacer = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TitleSpacer"));
	TitleSpacer->SetMinDesiredHeight(12.0f);
	RightVBox->AddChild(TitleSpacer);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(TitleSpacer->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// Dividing line between title and content
	USizeBox* DividerBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DividerSizeBox"));
	DividerBox->SetMinDesiredHeight(1.0f);
	DividerBox->SetMaxDesiredHeight(1.0f);
	RightVBox->AddChild(DividerBox);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(DividerBox->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		VSlot->SetHorizontalAlignment(HAlign_Fill);
		VSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}
	UImage* Divider = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleDivider"));
	Divider->SetColorAndOpacity(FLinearColor(0.25f, 0.3f, 0.27f, 0.6f));
	DividerBox->AddChild(Divider);

	// NoteContent (BindWidget) inside ScrollBox — centered, light weight font
	UScrollBox* ContentScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NoteContentScroll"));
	ContentScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	ContentScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	RightVBox->AddChild(ContentScroll);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(ContentScroll->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* NoteContent = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoteContent"));
	NoteContent->SetText(FText::FromString(TEXT("")));
	NoteContent->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.0f)));
	{
		FSlateFontInfo Font = NoteContent->GetFont();
		Font.Size = 16;
		Font.TypefaceFontName = FName("Light");
		NoteContent->SetFont(Font);
	}
	NoteContent->SetJustification(ETextJustify::Center);
	NoteContent->SetAutoWrapText(true);
	ContentScroll->AddChild(NoteContent);

	// Set new root
	Tree->RootWidget = Root;

	// --- Reparent to ZP_NotesWidget ---
	WBP->ParentClass = UZP_NotesWidget::StaticClass();

	// Compile and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetupNotesWidget: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// CreateNoteEntryWidget — create WBP_NoteEntry from scratch
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::CreateNoteEntryWidget()
{
	const FString PackagePath = TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/");
	const FString AssetName = TEXT("WBP_NoteEntry");
	const FString FullPath = PackagePath + AssetName;

	// Check if already exists
	if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, *(FullPath + TEXT(".") + AssetName)))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] CreateNoteEntryWidget: WBP_NoteEntry already exists — skipping"));
		return true;
	}

	// Create package
	UPackage* Package = CreatePackage(*(PackagePath + AssetName));
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] CreateNoteEntryWidget: Failed to create package"));
		return false;
	}

	// Create the WidgetBlueprint via factory
	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	Factory->ParentClass = UZP_NoteEntryWidget::StaticClass();

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(
		Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(), Package, FName(*AssetName),
			RF_Public | RF_Standalone, nullptr, GWarn));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] CreateNoteEntryWidget: Factory failed to create WBP_NoteEntry"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] CreateNoteEntryWidget: WidgetTree is null"));
		return false;
	}

	WBP->Modify();

	// Build structure:
	// SizeBox "EntrySizeBox" (min height for double-row entries)
	// └── Button "EntryButton" (flat dark bg)
	//     └── TextBlock "EntryTitle" (left-aligned)

	USizeBox* EntrySizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EntrySizeBox"));
	EntrySizeBox->SetMinDesiredHeight(56.0f);

	UButton* EntryButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EntryButton"));
	EntryButton->SetBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 0.6f));
	EntrySizeBox->AddChild(EntryButton);

	UTextBlock* EntryTitle = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EntryTitle"));
	EntryTitle->SetText(FText::FromString(TEXT("Note Title")));
	EntryTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 0.9f)));
	{
		FSlateFontInfo Font = EntryTitle->GetFont();
		Font.Size = 16;
		EntryTitle->SetFont(Font);
	}
	EntryTitle->SetJustification(ETextJustify::Left);

	EntryButton->AddChild(EntryTitle);
	Tree->RootWidget = EntrySizeBox;

	// Compile and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		Package,
		WBP,
		*FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(WBP);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CreateNoteEntryWidget: SUCCESS — WBP_NoteEntry created (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// SetNotesWidgetDefaults — wire NoteEntryWidgetClass on WBP_Notes
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::SetNotesWidgetDefaults()
{
	UWidgetBlueprint* NotesBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes"));

	if (!NotesBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetNotesWidgetDefaults: Failed to load WBP_Notes"));
		return false;
	}

	UWidgetBlueprint* EntryBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_NoteEntry.WBP_NoteEntry"));

	if (!EntryBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetNotesWidgetDefaults: Failed to load WBP_NoteEntry"));
		return false;
	}

	// Get the generated class (the TSubclassOf target)
	UClass* EntryClass = EntryBP->GeneratedClass;
	if (!EntryClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetNotesWidgetDefaults: WBP_NoteEntry has no GeneratedClass"));
		return false;
	}

	// Set CDO default
	UClass* NotesClass = NotesBP->GeneratedClass;
	if (!NotesClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetNotesWidgetDefaults: WBP_Notes has no GeneratedClass"));
		return false;
	}

	UObject* CDO = NotesClass->GetDefaultObject();
	FClassProperty* Prop = CastField<FClassProperty>(NotesClass->FindPropertyByName(TEXT("NoteEntryWidgetClass")));
	if (Prop)
	{
		Prop->SetObjectPropertyValue(Prop->ContainerPtrToValuePtr<void>(CDO), EntryClass);
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetNotesWidgetDefaults: NoteEntryWidgetClass set to %s"), *EntryClass->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetNotesWidgetDefaults: NoteEntryWidgetClass property not found"));
		return false;
	}

	// Save
	NotesBP->Modify();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(NotesBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		NotesBP->GetPackage(),
		NotesBP,
		*FPackageName::LongPackageNameToFilename(NotesBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetNotesWidgetDefaults: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// StripNotesBlueprint — remove old EGUI graphs, nodes, and variables
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::StripNotesBlueprint()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] StripNotesBlueprint: Failed to load WBP_Notes"));
		return false;
	}

	WBP->Modify();
	int32 RemovedNodes = 0;
	int32 RemovedGraphs = 0;
	int32 RemovedVars = 0;

	// Properly destroy all nodes in each UberGraph page using RemoveNode
	// (Graph->Nodes.Empty() only clears the array, leaving node objects serialized in the package)
	for (UEdGraph* Graph : WBP->UbergraphPages)
	{
		if (Graph)
		{
			Graph->Modify();
			TArray<UEdGraphNode*> NodesToRemove = Graph->Nodes;
			RemovedNodes += NodesToRemove.Num();
			for (UEdGraphNode* Node : NodesToRemove)
			{
				if (Node)
				{
					Graph->RemoveNode(Node);
				}
			}
		}
	}

	// Remove all UberGraph pages except keep one clean EventGraph
	if (WBP->UbergraphPages.Num() > 1)
	{
		// Keep only the first page (the main EventGraph), remove all others
		TArray<UEdGraph*> PagesToRemove;
		for (int32 i = 1; i < WBP->UbergraphPages.Num(); ++i)
		{
			PagesToRemove.Add(WBP->UbergraphPages[i]);
		}
		for (UEdGraph* Page : PagesToRemove)
		{
			FBlueprintEditorUtils::RemoveGraph(WBP, Page);
			RemovedGraphs++;
		}
	}

	// Remove all function graphs — must use RemoveGraph to destroy properly
	{
		TArray<UEdGraph*> FuncGraphs = WBP->FunctionGraphs;
		RemovedGraphs += FuncGraphs.Num();
		for (UEdGraph* Graph : FuncGraphs)
		{
			if (Graph)
			{
				FBlueprintEditorUtils::RemoveGraph(WBP, Graph);
			}
		}
	}

	// Remove all Blueprint-defined variables (NewVariables)
	RemovedVars = WBP->NewVariables.Num();
	WBP->NewVariables.Empty();

	// Remove delegate graphs properly
	{
		TArray<UEdGraph*> DelGraphs = WBP->DelegateSignatureGraphs;
		RemovedGraphs += DelGraphs.Num();
		for (UEdGraph* Graph : DelGraphs)
		{
			if (Graph)
			{
				FBlueprintEditorUtils::RemoveGraph(WBP, Graph);
			}
		}
	}

	// Remove all implemented interfaces from the old EGUI parent
	WBP->ImplementedInterfaces.Empty();

	// Compile and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] StripNotesBlueprint: Removed %d nodes, %d graphs, %d variables (saved=%s)"),
		RemovedNodes, RemovedGraphs, RemovedVars, bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// RebuildNotesWidgetTree — clean rebuild without old EGUI remnants
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::RebuildNotesWidgetTree()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RebuildNotesWidgetTree: Failed to load WBP_Notes"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RebuildNotesWidgetTree: WidgetTree is null"));
		return false;
	}

	WBP->Modify();

	// Destroy the entire existing widget tree
	TArray<UWidget*> AllWidgets;
	Tree->GetAllWidgets(AllWidgets);
	for (UWidget* W : AllWidgets)
	{
		Tree->RemoveWidget(W);
	}
	Tree->RootWidget = nullptr;

	// Build fresh structure:
	// Overlay (root)
	// ├── Image "NotesBG" (dark semi-transparent background)
	// └── HorizontalBox "NotesContentHBox"
	//     ├── SizeBox (left, width=250)
	//     │   └── Overlay
	//     │       ├── ScrollBox "NoteListScrollBox"
	//     │       └── TextBlock "NotesEmptyText"
	//     └── VerticalBox (right, fills remaining)
	//         ├── TextBlock "NoteTitle"
	//         └── ScrollBox wrapping TextBlock "NoteContent"

	UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("NotesRoot"));

	// Semi-transparent dark background matching the map panel aesthetic
	UImage* BG = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NotesBG"));
	BG->SetColorAndOpacity(FLinearColor(0.01f, 0.01f, 0.02f, 0.75f));
	Root->AddChild(BG);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(BG->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Content HorizontalBox
	UHorizontalBox* ContentHBox = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NotesContentHBox"));
	Root->AddChild(ContentHBox);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(ContentHBox->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// === LEFT PANEL (note list) ===
	USizeBox* LeftSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LeftPanelSizeBox"));
	ContentHBox->AddChild(LeftSizeBox);
	LeftSizeBox->SetWidthOverride(300.0f);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(LeftSizeBox->Slot))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(8.0f, 8.0f, 4.0f, 8.0f));
	}

	UOverlay* LeftOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LeftPanelOverlay"));
	LeftSizeBox->AddChild(LeftOverlay);

	// NoteListScrollBox (BindWidget)
	UScrollBox* NoteListScrollBox = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NoteListScrollBox"));
	LeftOverlay->AddChild(NoteListScrollBox);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(NoteListScrollBox->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// NotesEmptyText (BindWidgetOptional)
	UTextBlock* NotesEmptyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NotesEmptyText"));
	NotesEmptyText->SetText(FText::FromString(TEXT("No notes collected")));
	NotesEmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 0.6f)));
	{
		FSlateFontInfo Font = NotesEmptyText->GetFont();
		Font.Size = 14;
		NotesEmptyText->SetFont(Font);
	}
	NotesEmptyText->SetJustification(ETextJustify::Center);
	LeftOverlay->AddChild(NotesEmptyText);
	if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(NotesEmptyText->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Center);
		OSlot->SetVerticalAlignment(VAlign_Center);
	}

	// === RIGHT PANEL (note content) ===
	UVerticalBox* RightVBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightPanelVBox"));
	ContentHBox->AddChild(RightVBox);
	if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(RightVBox->Slot))
	{
		HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HSlot->SetVerticalAlignment(VAlign_Fill);
		HSlot->SetPadding(FMargin(4.0f, 8.0f, 8.0f, 8.0f));
	}

	// NoteTitle (BindWidget)
	UTextBlock* NoteTitle = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoteTitle"));
	NoteTitle->SetText(FText::GetEmpty());
	NoteTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f)));
	{
		FSlateFontInfo Font = NoteTitle->GetFont();
		Font.Size = 20;
		NoteTitle->SetFont(Font);
	}
	RightVBox->AddChild(NoteTitle);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(NoteTitle->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		VSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	// NoteContent inside a ScrollBox so long notes are scrollable
	UScrollBox* ContentScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NoteContentScroll"));
	RightVBox->AddChild(ContentScroll);
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(ContentScroll->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* NoteContent = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoteContent"));
	NoteContent->SetText(FText::GetEmpty());
	NoteContent->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
	{
		FSlateFontInfo Font = NoteContent->GetFont();
		Font.Size = 14;
		NoteContent->SetFont(Font);
	}
	NoteContent->SetAutoWrapText(true);
	ContentScroll->AddChild(NoteContent);

	// Set root
	Tree->RootWidget = Root;

	// Ensure parent class is correct
	WBP->ParentClass = UZP_NotesWidget::StaticClass();

	// Compile and save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] RebuildNotesWidgetTree: SUCCESS — clean widget tree built (saved=%s)"),
		bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// StripNotesWidgetChildren — remove all EGUI widget children except background
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::StripNotesWidgetChildren()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes.WBP_Notes"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] StripNotesWidgetChildren: Failed to load WBP_Notes"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree || !Tree->RootWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] StripNotesWidgetChildren: No widget tree or root"));
		return false;
	}

	WBP->Modify();

	// Log the tree structure for diagnostics
	TArray<UWidget*> AllWidgets;
	Tree->GetAllWidgets(AllWidgets);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] StripNotesWidgetChildren: Tree has %d widgets"), AllWidgets.Num());
	for (UWidget* W : AllWidgets)
	{
		UWidget* Parent = W->GetParent();
		UE_LOG(LogTemp, Log, TEXT("  Widget: %s (%s) — Parent: %s"),
			*W->GetName(), *W->GetClass()->GetName(),
			Parent ? *Parent->GetName() : TEXT("ROOT"));
	}

	UPanelWidget* Root = Cast<UPanelWidget>(Tree->RootWidget);
	if (!Root)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] StripNotesWidgetChildren: Root is not a panel widget"));
		return false;
	}

	// The root is typically a CanvasPanel. Its children include the CommonBackground
	// and all the settings UI (tabs, panels, etc.). We want to keep ONLY the root
	// and its first child (the background). Remove everything else.

	int32 ChildCount = Root->GetChildrenCount();
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Root '%s' has %d direct children"), *Root->GetName(), ChildCount);

	if (ChildCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] StripNotesWidgetChildren: Root has no children"));
		return true;
	}

	// Log all direct children of root
	for (int32 i = 0; i < ChildCount; ++i)
	{
		UWidget* Child = Root->GetChildAt(i);
		if (Child)
		{
			UE_LOG(LogTemp, Log, TEXT("  [%d] %s (%s)"), i, *Child->GetName(), *Child->GetClass()->GetName());
		}
	}

	// Keep child[0] (the background), remove all others
	UWidget* Background = Root->GetChildAt(0);
	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Keeping background: %s (%s)"),
		*Background->GetName(), *Background->GetClass()->GetName());

	// Collect children to remove (skip index 0)
	TArray<UWidget*> ToRemove;
	for (int32 i = 1; i < ChildCount; ++i)
	{
		UWidget* Child = Root->GetChildAt(i);
		if (Child)
		{
			ToRemove.Add(Child);
		}
	}

	// Remove them from root
	for (UWidget* W : ToRemove)
	{
		Root->RemoveChild(W);
		Tree->RemoveWidget(W);
	}

	// If the background itself is a panel, strip ITS children too
	// (we want just the visual background, not any content inside it)
	if (UPanelWidget* BgPanel = Cast<UPanelWidget>(Background))
	{
		int32 BgChildCount = BgPanel->GetChildrenCount();
		if (BgChildCount > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[TheSignal] Background '%s' has %d children — stripping them"),
				*Background->GetName(), BgChildCount);

			TArray<UWidget*> BgChildren;
			for (int32 i = 0; i < BgChildCount; ++i)
			{
				UWidget* C = BgPanel->GetChildAt(i);
				if (C) BgChildren.Add(C);
			}
			for (UWidget* C : BgChildren)
			{
				BgPanel->RemoveChild(C);
				Tree->RemoveWidget(C);
			}
		}
	}

	// Also remove any orphaned widgets not attached to root
	TArray<UWidget*> Remaining;
	Tree->GetAllWidgets(Remaining);
	int32 Orphans = 0;
	for (UWidget* W : Remaining)
	{
		if (W != Root && W != Background)
		{
			Tree->RemoveWidget(W);
			Orphans++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] Removed %d root children, %d orphans. Tree now has root + background only."),
		ToRemove.Num(), Orphans);

	// Save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] StripNotesWidgetChildren: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

// ---------------------------------------------------------------------------
// WrapPickupDescriptionInScrollBox — make item pickup description scrollable
// ---------------------------------------------------------------------------

bool UZP_EditorWidgetUtils::WrapPickupDescriptionInScrollBox()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] WrapPickupDesc: Failed to load WBP"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] WrapPickupDesc: WidgetTree is null"));
		return false;
	}

	// Find the description rich text widget
	UWidget* DescWidget = Tree->FindWidget(TEXT("ItemDescriptionRichText"));
	if (!DescWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] WrapPickupDesc: ItemDescriptionRichText not found"));
		return false;
	}

	// Check if already wrapped in a ScrollBox
	UPanelWidget* Parent = DescWidget->GetParent();
	if (!Parent)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] WrapPickupDesc: Description has no parent"));
		return false;
	}

	// Check if already wrapped
	if (Tree->FindWidget(TEXT("DescriptionScrollBox")))
	{
		UE_LOG(LogTemp, Log, TEXT("[TheSignal] WrapPickupDesc: Already wrapped — removing old wrap first"));
		// Remove old wrap — just proceed with fresh wrap below
		// The old ScrollBox/SizeBox will be orphaned but harmless
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] WrapPickupDesc: Parent is %s (children=%d), wrapping in SizeBox+ScrollBox"),
		*Parent->GetClass()->GetName(), Parent->GetChildrenCount());

	WBP->Modify();

	// Remove description from its parent
	Parent->RemoveChild(DescWidget);

	// Create SizeBox with explicit max height — guarantees scroll constraint
	// regardless of parent layout type
	USizeBox* DescSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DescriptionSizeBox"));
	DescSizeBox->SetMaxDesiredHeight(350.0f);

	// Create ScrollBox inside SizeBox
	UScrollBox* DescScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DescriptionScrollBox"));
	DescScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	DescScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);

	// Wire: Parent > SizeBox > ScrollBox > DescriptionText
	Parent->AddChild(DescSizeBox);

	// Set slot on SizeBox based on parent type
	if (UVerticalBoxSlot* VSlot = Cast<UVerticalBoxSlot>(DescSizeBox->Slot))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		VSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	else if (UOverlaySlot* OSlot = Cast<UOverlaySlot>(DescSizeBox->Slot))
	{
		OSlot->SetHorizontalAlignment(HAlign_Fill);
		OSlot->SetVerticalAlignment(VAlign_Fill);
	}

	DescSizeBox->AddChild(DescScroll);
	DescScroll->AddChild(DescWidget);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] WrapPickupDesc: Slot type after AddChild: %s"),
		DescSizeBox->Slot ? *DescSizeBox->Slot->GetClass()->GetName() : TEXT("NULL"));

	// Save
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] WrapPickupDesc: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

FString UZP_EditorWidgetUtils::DumpPickupWidgetTree()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] DumpPickupWidgetTree: Failed to load WBP"));
		return TEXT("ERROR: Failed to load");
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] DumpPickupWidgetTree: WidgetTree is null"));
		return TEXT("ERROR: WidgetTree null");
	}

	FString Result;

	// Recursive lambda to dump tree
	TFunction<void(UWidget*, int32)> DumpWidget = [&](UWidget* Widget, int32 Depth)
	{
		if (!Widget) return;

		FString Indent = FString::ChrN(Depth * 2, TEXT(' '));
		FString Line = FString::Printf(TEXT("%s%s (%s)"),
			*Indent,
			*Widget->GetName(),
			*Widget->GetClass()->GetName());

		// Check if marked as variable
		if (Widget->bIsVariable)
		{
			Line += TEXT(" [IS_VARIABLE]");
		}

		UE_LOG(LogTemp, Log, TEXT("[WidgetTree] %s"), *Line);
		Result += Line + TEXT("\n");

		// Recurse into children
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
			{
				DumpWidget(Panel->GetChildAt(i), Depth + 1);
			}
		}
	};

	UWidget* Root = Tree->RootWidget;
	if (Root)
	{
		DumpWidget(Root, 0);
	}
	else
	{
		Result = TEXT("ERROR: No root widget");
	}

	return Result;
}

bool UZP_EditorWidgetUtils::RestorePickupTextWidgets()
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror"));

	if (!WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: Failed to load WBP"));
		return false;
	}

	UWidgetTree* Tree = WBP->WidgetTree;
	if (!Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: WidgetTree is null"));
		return false;
	}

	// Find InfoVBox — the vertical box that should contain title + separator + description
	UVerticalBox* InfoVBox = Cast<UVerticalBox>(Tree->FindWidget(TEXT("InfoVBox")));
	if (!InfoVBox)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: InfoVBox not found"));
		return false;
	}

	// Find CommonTextBlock and CommonRichTextBlock classes via reflection (avoids CommonUI dependency)
	UClass* CommonTextBlockClass = FindObject<UClass>(nullptr, TEXT("/Script/CommonUI.CommonTextBlock"));
	UClass* CommonRichTextBlockClass = FindObject<UClass>(nullptr, TEXT("/Script/CommonUI.CommonRichTextBlock"));

	if (!CommonTextBlockClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: CommonTextBlock class not found"));
		return false;
	}
	if (!CommonRichTextBlockClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: CommonRichTextBlock class not found"));
		return false;
	}

	// Check if widgets already exist (idempotency!)
	UWidget* ExistingName = Tree->FindWidget(TEXT("ItemNameText2"));
	UWidget* ExistingDesc = Tree->FindWidget(TEXT("ItemDescriptionRichText"));
	if (ExistingName && ExistingDesc)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TheSignal] RestorePickupTextWidgets: Widgets already exist, skipping"));
		return true;
	}

	// Snapshot current InfoVBox children before modification
	TArray<UWidget*> ExistingChildren;
	for (int32 i = 0; i < InfoVBox->GetChildrenCount(); i++)
	{
		ExistingChildren.Add(InfoVBox->GetChildAt(i));
	}

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] RestorePickupTextWidgets: InfoVBox has %d existing children"), ExistingChildren.Num());

	// Create ItemNameText2 (CommonTextBlock) — for the note title
	UWidget* NameWidget = Tree->ConstructWidget<UWidget>(CommonTextBlockClass, TEXT("ItemNameText2"));
	if (!NameWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: Failed to create ItemNameText2"));
		return false;
	}
	NameWidget->bIsVariable = true;

	// Create ItemDescriptionRichText (CommonRichTextBlock) — for the note content
	UWidget* DescWidget = Tree->ConstructWidget<UWidget>(CommonRichTextBlockClass, TEXT("ItemDescriptionRichText"));
	if (!DescWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] RestorePickupTextWidgets: Failed to create ItemDescriptionRichText"));
		return false;
	}
	DescWidget->bIsVariable = true;

	// Remove existing children from InfoVBox temporarily
	for (UWidget* Child : ExistingChildren)
	{
		InfoVBox->RemoveChild(Child);
	}

	// Rebuild InfoVBox: NameText → existing children (separator) → DescriptionText
	InfoVBox->AddChild(NameWidget);
	// Set name text alignment — center, fill width
	if (UVerticalBoxSlot* NameSlot = Cast<UVerticalBoxSlot>(NameWidget->Slot))
	{
		NameSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
		NameSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));
	}

	// Re-add existing children (separator lines)
	for (UWidget* Child : ExistingChildren)
	{
		InfoVBox->AddChild(Child);
	}

	// Add description at the end
	InfoVBox->AddChild(DescWidget);
	if (UVerticalBoxSlot* DescSlot = Cast<UVerticalBoxSlot>(DescWidget->Slot))
	{
		DescSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
		DescSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Top);
		DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DescSlot->SetPadding(FMargin(16.0f, 8.0f, 16.0f, 8.0f));
	}

	// Mark blueprint as modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);

	// Save
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] RestorePickupTextWidgets: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

bool UZP_EditorWidgetUtils::CopyPickupTextStyles()
{
	// Load both BPs
	UWidgetBlueprint* RPG_WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/RPG/UI/Widgets/WBP_FirstTimePickupNotification_RPG.WBP_FirstTimePickupNotification_RPG"));
	UWidgetBlueprint* Horror_WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror"));

	if (!RPG_WBP || !Horror_WBP)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] CopyPickupTextStyles: Failed to load one or both BPs"));
		return false;
	}

	UWidgetTree* RPG_Tree = RPG_WBP->WidgetTree;
	UWidgetTree* Horror_Tree = Horror_WBP->WidgetTree;
	if (!RPG_Tree || !Horror_Tree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] CopyPickupTextStyles: Missing widget tree"));
		return false;
	}

	// Copy properties for each widget pair
	TArray<FName> WidgetNames = { TEXT("ItemNameText2"), TEXT("ItemDescriptionRichText") };

	for (const FName& WidgetName : WidgetNames)
	{
		UWidget* RPG_Widget = RPG_Tree->FindWidget(WidgetName);
		UWidget* Horror_Widget = Horror_Tree->FindWidget(WidgetName);

		if (!RPG_Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("[TheSignal] CopyPickupTextStyles: RPG missing widget %s"), *WidgetName.ToString());
			continue;
		}
		if (!Horror_Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("[TheSignal] CopyPickupTextStyles: Horror missing widget %s"), *WidgetName.ToString());
			continue;
		}

		// Both must be same class
		if (RPG_Widget->GetClass() != Horror_Widget->GetClass())
		{
			UE_LOG(LogTemp, Error, TEXT("[TheSignal] CopyPickupTextStyles: Class mismatch for %s: %s vs %s"),
				*WidgetName.ToString(),
				*RPG_Widget->GetClass()->GetName(),
				*Horror_Widget->GetClass()->GetName());
			continue;
		}

		// Copy style properties — allow struct properties and specific object refs
		UClass* WidgetClass = RPG_Widget->GetClass();

		// Blocklist: properties that must NOT be copied (structural/identity)
		TSet<FString> BlockedProps = {
			TEXT("Slot"), TEXT("bIsVariable"), TEXT("bIsEnabled"),
			TEXT("bIsVolatile"), TEXT("Visibility"), TEXT("RenderTransform"),
			TEXT("Cursor"), TEXT("Navigation"), TEXT("FlowDirectionPreference"),
			TEXT("AccessibleText"), TEXT("AccessibleSummaryText"),
			TEXT("ToolTipText"), TEXT("ToolTipWidget"), TEXT("bCreatedByConstructionScript"),
			TEXT("bExpandedInDesigner"), TEXT("bLockedInDesigner"),
			TEXT("DesignerFlags"), TEXT("DisplayLabel"), TEXT("CategoryName")
		};

		// Specifically allowed object reference properties (DataTable refs are safe)
		TSet<FString> AllowedObjectProps = {
			TEXT("TextStyleSet"), TEXT("DecoratorClasses")
		};

		int32 CopiedCount = 0;
		for (TFieldIterator<FProperty> PropIt(WidgetClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			FString PropName = Prop->GetName();

			// Skip blocked properties
			if (BlockedProps.Contains(PropName))
				continue;

			// Skip UWidget base structural properties (anything defined on UWidget or above)
			UClass* PropOwner = Prop->GetOwnerClass();
			if (PropOwner && (PropOwner == UWidget::StaticClass() ||
				PropOwner->IsChildOf(UObject::StaticClass()) && !PropOwner->IsChildOf(UWidget::StaticClass())))
			{
				// Allow if it's specifically allowlisted
				if (!AllowedObjectProps.Contains(PropName))
					continue;
			}

			// For object references, only allow specifically listed ones
			if (Prop->IsA<FObjectProperty>() && !AllowedObjectProps.Contains(PropName))
				continue;

			// For array of object references, only allow specifically listed
			if (Prop->IsA<FArrayProperty>())
			{
				FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
				if (ArrayProp->Inner && ArrayProp->Inner->IsA<FObjectProperty>() && !AllowedObjectProps.Contains(PropName))
					continue;
			}

			const void* SrcValue = Prop->ContainerPtrToValuePtr<void>(RPG_Widget);
			void* DstValue = Prop->ContainerPtrToValuePtr<void>(Horror_Widget);
			Prop->CopyCompleteValue(DstValue, SrcValue);
			CopiedCount++;

			UE_LOG(LogTemp, Log, TEXT("[TheSignal] CopyPickupTextStyles: Copied %s.%s (%s)"),
				*WidgetName.ToString(), *PropName, *Prop->GetClass()->GetName());
		}

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] CopyPickupTextStyles: Copied %d properties for %s (%s)"),
			CopiedCount, *WidgetName.ToString(), *WidgetClass->GetName());
	}

	// Mark modified and compile
	FBlueprintEditorUtils::MarkBlueprintAsModified(Horror_WBP);
	FKismetEditorUtilities::CompileBlueprint(Horror_WBP);

	// Save
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	bool bSaved = UPackage::SavePackage(
		Horror_WBP->GetPackage(),
		Horror_WBP,
		*FPackageName::LongPackageNameToFilename(Horror_WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("[TheSignal] CopyPickupTextStyles: SUCCESS (saved=%s)"), bSaved ? TEXT("true") : TEXT("false"));
	return true;
}

bool UZP_EditorWidgetUtils::SetPickupDescriptionFontSize(int32 FontSize)
{
	UWidgetBlueprint* WBP = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror.WBP_FirstTimePickupNotification_Horror"));

	if (!WBP || !WBP->WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetPickupDescriptionFontSize: Failed to load WBP or tree"));
		return false;
	}

	// Find the description widget
	UWidget* DescWidget = WBP->WidgetTree->FindWidget(TEXT("ItemDescriptionRichText"));
	if (!DescWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetPickupDescriptionFontSize: ItemDescriptionRichText not found"));
		return false;
	}

	// Set DefaultTextStyleOverride via reflection (avoids CommonUI header dependency)
	// URichTextBlock has DefaultTextStyleOverride (FTextBlockStyle)
	FProperty* StyleProp = DescWidget->GetClass()->FindPropertyByName(TEXT("DefaultTextStyleOverride"));
	if (!StyleProp)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetPickupDescriptionFontSize: DefaultTextStyleOverride property not found"));
		return false;
	}

	// Get the FTextBlockStyle value
	FStructProperty* StructProp = CastField<FStructProperty>(StyleProp);
	if (!StructProp)
	{
		UE_LOG(LogTemp, Error, TEXT("[TheSignal] SetPickupDescriptionFontSize: Not a struct property"));
		return false;
	}

	void* StylePtr = StructProp->ContainerPtrToValuePtr<void>(DescWidget);
	FTextBlockStyle* TextStyle = static_cast<FTextBlockStyle*>(StylePtr);

	if (TextStyle)
	{
		// Set font size
		TextStyle->Font.Size = FontSize;
		// Use Roboto as a safe default
		TextStyle->Font.FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		TextStyle->ColorAndOpacity = FSlateColor(FLinearColor::White);

		UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetPickupDescriptionFontSize: Set font size to %d"), FontSize);
	}

	// Also set font on the title widget (CommonTextBlock — inherits UTextBlock, NOT URichTextBlock)
	// UTextBlock uses "Font" property (FSlateFontInfo), not DefaultTextStyleOverride.
	UWidget* NameWidget = WBP->WidgetTree->FindWidget(TEXT("ItemNameText2"));
	if (NameWidget)
	{
		// CommonTextBlock has a Style (TSubclassOf<UCommonTextStyle>) that overrides Font at runtime.
		// Clear it so our Font setting takes effect.
		FProperty* StyleClassProp = NameWidget->GetClass()->FindPropertyByName(TEXT("Style"));
		if (StyleClassProp)
		{
			// Set the TSubclassOf to nullptr (clears the style override)
			void* StyleAddr = StyleClassProp->ContainerPtrToValuePtr<void>(NameWidget);
			FClassProperty* ClassProp = CastField<FClassProperty>(StyleClassProp);
			if (ClassProp)
			{
				ClassProp->SetObjectPropertyValue(StyleAddr, nullptr);
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetPickupDescriptionFontSize: Cleared CommonTextStyle on title"));
			}
		}

		// Set Font directly (UTextBlock property)
		FStructProperty* FontProp = CastField<FStructProperty>(NameWidget->GetClass()->FindPropertyByName(TEXT("Font")));
		if (FontProp)
		{
			FSlateFontInfo* FontInfo = FontProp->ContainerPtrToValuePtr<FSlateFontInfo>(NameWidget);
			if (FontInfo)
			{
				FontInfo->Size = FontSize;
				FontInfo->FontObject = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
				FontInfo->TypefaceFontName = FName("Bold");
				UE_LOG(LogTemp, Log, TEXT("[TheSignal] SetPickupDescriptionFontSize: Set title Font.Size=%d Bold"), FontSize);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TheSignal] SetPickupDescriptionFontSize: 'Font' property not found on %s"), *NameWidget->GetClass()->GetName());
		}
	}

	// Compile and save
	FBlueprintEditorUtils::MarkBlueprintAsModified(WBP);
	FKismetEditorUtilities::CompileBlueprint(WBP);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	UPackage::SavePackage(
		WBP->GetPackage(),
		WBP,
		*FPackageName::LongPackageNameToFilename(WBP->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()),
		SaveArgs);

	return true;
}

#endif // WITH_EDITOR
