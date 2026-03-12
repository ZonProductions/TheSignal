// Copyright The Signal. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZP_EditorWidgetUtils.generated.h"

/**
 * Editor-only utility for programmatically modifying Widget Blueprint trees.
 * Call from Python: unreal.ZP_EditorWidgetUtils.add_tabs_to_inventory_menu()
 */
UCLASS()
class UZP_EditorWidgetUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Adds Map/Inventory/Notes tabs to WBP_InventoryMenu_Horror.
	 * Wraps existing root in a WidgetSwitcher (index 1 = inventory),
	 * adds Map panel (index 0) and Notes panel (index 2),
	 * and a tab bar with 3 buttons at the top.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool AddTabsToInventoryMenu();

	/**
	 * Sets up WBP_Notes widget tree with BindWidget-compatible widgets:
	 *   Overlay root > HorizontalBox > [left: ScrollBox "NoteListScrollBox" + TextBlock "NotesEmptyText"]
	 *                                  [right: VerticalBox > TextBlock "NoteTitle" + TextBlock "NoteContent"]
	 * Then reparents WBP_Notes to ZP_NotesWidget.
	 * Preserves the existing BackgroundImage as the first overlay child.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool SetupNotesWidget();

	/**
	 * Creates WBP_NoteEntry widget blueprint with BindWidget-compatible widgets:
	 *   Button "EntryButton" > TextBlock "EntryTitle"
	 * Parent class: ZP_NoteEntryWidget.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool CreateNoteEntryWidget();

	/**
	 * Sets WBP_Notes default: NoteEntryWidgetClass = WBP_NoteEntry.
	 * Call AFTER CreateNoteEntryWidget() and SetupNotesWidget().
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool SetNotesWidgetDefaults();

	/**
	 * Strips all old EGUI options menu event graph nodes, function graphs,
	 * and variables from WBP_Notes. Leaves only the widget tree and parent class.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool StripNotesBlueprint();

	/**
	 * Rebuilds WBP_Notes widget tree from scratch (destroys existing tree).
	 * Clean two-column layout with dark background — no old EGUI remnants.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool RebuildNotesWidgetTree();

	/**
	 * Strips ALL child widgets from WBP_Notes widget tree EXCEPT the
	 * background widget (first child of root). Logs the tree structure
	 * before stripping for diagnostics. Call AFTER StripNotesBlueprint()
	 * and BEFORE SetupNotesWidget().
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool StripNotesWidgetChildren();

	/**
	 * Wraps the ItemDescriptionRichText widget in WBP_FirstTimePickupNotification_Horror
	 * inside a ScrollBox so long note content is scrollable on item pickup.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool WrapPickupDescriptionInScrollBox();

	/**
	 * Logs all widget names + classes in the WBP_FirstTimePickupNotification_Horror widget tree.
	 * Diagnostic only — does NOT modify anything.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static FString DumpPickupWidgetTree();

	/**
	 * Adds missing text widgets (ItemNameText2, ItemDescriptionRichText) to the
	 * WBP_FirstTimePickupNotification_Horror widget tree. These were lost during
	 * asset migration. Inserts into InfoVBox at the correct positions.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool RestorePickupTextWidgets();

	/**
	 * Copies text widget style properties from RPG variant to Horror variant.
	 * Fixes missing font/style on CommonRichTextBlock after widget recreation.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool CopyPickupTextStyles();

	/**
	 * Sets font size on the pickup notification's description text widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "TheSignal|Editor")
	static bool SetPickupDescriptionFontSize(int32 FontSize = 16);
};

#endif
