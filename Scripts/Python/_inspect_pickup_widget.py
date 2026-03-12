import unreal

bp = unreal.load_asset("/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror")
if not bp:
    print("ERROR: Could not load WBP_FirstTimePickupNotification_Horror")
else:
    tree = bp.get_editor_property("widget_tree")
    root = tree.get_editor_property("root_widget")

    def dump_tree(widget, indent=0):
        if not widget:
            return
        name = widget.get_name()
        cls = widget.get_class().get_name()
        slot_info = ""
        print(f"{'  ' * indent}{name} ({cls}){slot_info}")

        # Check if it's a panel that has children
        if hasattr(widget, 'get_all_children'):
            children = widget.get_all_children()
            for child in children:
                dump_tree(child, indent + 1)
        elif hasattr(widget, 'get_content'):
            content = widget.get_content()
            if content:
                dump_tree(content, indent + 1)

    print("=== Widget Tree ===")
    dump_tree(root)
