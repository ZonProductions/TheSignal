import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if bp is None:
    print("ERROR: Could not load WBP_Notes")
else:
    print(f"Loaded: {bp.get_name()}, Class: {bp.get_class().get_name()}")
    gen = bp.generated_class()
    if gen:
        parent_names = []
        cur = unreal.SystemLibrary.get_super_class(gen) if hasattr(unreal.SystemLibrary, 'get_super_class') else None
        print(f"GeneratedClass: {gen.get_name()}")

    # Access widget tree
    wt = bp.get_editor_property('widget_tree')
    if wt is None:
        print("ERROR: No widget_tree property")
    else:
        root = wt.get_editor_property('root_widget')
        if root is None:
            print("ERROR: No root widget")
        else:
            def dump_tree(widget, indent=0):
                prefix = "  " * indent
                name = widget.get_name()
                cls = widget.get_class().get_name()
                vis = str(widget.get_editor_property('visibility')) if hasattr(widget, 'get_editor_property') else '?'
                print(f"{prefix}{name} ({cls})")
                # PanelWidget.get_all_children()
                try:
                    count = widget.get_child_count()
                    for i in range(count):
                        child = widget.get_child_at(i)
                        dump_tree(child, indent + 1)
                except:
                    pass

            dump_tree(root)
