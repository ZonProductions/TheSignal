// Copyright The Signal. All Rights Reserved.

#if WITH_EDITOR

#include "ZP_EditorWidgetUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
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
#include "Components/ScrollBox.h"

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

#endif // WITH_EDITOR
