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
};

#endif
