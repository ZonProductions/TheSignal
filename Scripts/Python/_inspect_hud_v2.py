import unreal

wbp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
if not wbp:
    print("NOT FOUND")
else:
    # WidgetBlueprint has a WidgetTree
    tree = wbp.get_editor_property("widget_tree")
    if tree:
        root = tree.get_editor_property("root_widget")
        print(f"Root: {root.get_name()} ({root.get_class().get_name()})" if root else "No root")

        # Get all named widgets
        all_widgets = tree.get_editor_property("all_widgets")
        if all_widgets:
            for w in all_widgets:
                print(f"  {w.get_name()} ({w.get_class().get_name()})")
        else:
            # Try iterating the root
            print("Trying manual traversal...")
    else:
        print("No widget tree")
