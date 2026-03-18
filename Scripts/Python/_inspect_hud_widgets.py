import unreal

# Load the widget blueprint
wbp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
if not wbp:
    print("WBP_HUD not found")
else:
    cdo = unreal.get_default_object(wbp.generated_class())
    # Try to find HealthArc and StaminaArc properties
    try:
        health_arc = cdo.get_editor_property("HealthArc")
        print(f"HealthArc: {health_arc}")
        if health_arc:
            slot = health_arc.slot
            print(f"  Slot type: {type(slot)}")
    except Exception as e:
        print(f"HealthArc error: {e}")

    try:
        stamina_arc = cdo.get_editor_property("StaminaArc")
        print(f"StaminaArc: {stamina_arc}")
    except Exception as e:
        print(f"StaminaArc error: {e}")

    # List widget tree via WidgetTree
    try:
        tree = cdo.widget_tree
        if tree:
            root = tree.root_widget
            print(f"Root widget: {root.get_name() if root else 'None'}")

            def print_tree(widget, indent=0):
                if not widget:
                    return
                name = widget.get_name()
                cls = widget.get_class().get_name()
                print(f"{'  ' * indent}{name} ({cls})")
                # Try to get children
                try:
                    panel = unreal.PanelWidget.cast(widget)
                    if panel:
                        for i in range(panel.get_child_renumber()):
                            print_tree(panel.get_child_at(i), indent + 1)
                except:
                    pass

            print_tree(root)
    except Exception as e:
        print(f"Widget tree error: {e}")
