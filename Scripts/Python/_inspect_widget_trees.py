"""Inspect WBP_Notes widget tree and MapImage slot in WBP_InventoryMenu_Horror."""
import unreal

def dump_tree(widget, indent=0):
    """Recursively dump widget tree."""
    prefix = '  ' * indent
    name = widget.get_name()
    cls = widget.get_class().get_name()
    vis = str(widget.get_editor_property('visibility')) if hasattr(widget, 'get_editor_property') else '?'
    slot_info = ''
    slot = widget.slot
    if slot:
        slot_cls = slot.get_class().get_name()
        slot_info = f' [slot: {slot_cls}]'
    print(f'{prefix}{name} ({cls}){slot_info}')

    # Check if it's a panel that has children
    try:
        count = widget.get_child_count() if hasattr(widget, 'get_child_count') else 0
        for i in range(count):
            child = widget.get_child_at(i)
            if child:
                dump_tree(child, indent + 1)
    except:
        pass

# === WBP_Notes ===
print('=== WBP_Notes Widget Tree ===')
notes_bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
if notes_bp:
    tree = notes_bp.get_editor_property('widget_tree')
    if tree:
        root = tree.root_widget
        if root:
            dump_tree(root)
        else:
            print('  No root widget')
    else:
        print('  No widget tree')

# === WBP_InventoryMenu_Horror — MapImage slot ===
print('\n=== WBP_InventoryMenu_Horror — MapImage ===')
inv_bp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
if inv_bp:
    tree2 = inv_bp.get_editor_property('widget_tree')
    if tree2:
        map_img = tree2.find_widget(unreal.Name('MapImage'))
        if map_img:
            print(f'MapImage found: {map_img.get_class().get_name()}')
            parent = map_img.get_parent()
            if parent:
                print(f'Parent: {parent.get_name()} ({parent.get_class().get_name()})')
            slot = map_img.slot
            if slot:
                print(f'Slot: {slot.get_class().get_name()}')
                # CanvasPanelSlot properties
                for prop_name in ['anchors', 'position', 'size', 'alignment', 'auto_size', 'z_order']:
                    try:
                        val = slot.get_editor_property(prop_name)
                        print(f'  {prop_name}: {val}')
                    except Exception as e:
                        print(f'  {prop_name}: ERROR {e}')
                # Anchors detail
                try:
                    a = slot.get_editor_property('anchors')
                    print(f'  anchors.minimum: ({a.minimum.x}, {a.minimum.y})')
                    print(f'  anchors.maximum: ({a.maximum.x}, {a.maximum.y})')
                except:
                    pass
                # Offsets
                try:
                    o = slot.get_editor_property('offsets')
                    print(f'  offsets: {o}')
                except:
                    pass
        else:
            print('MapImage not found')
            # List all widgets to find it
            root2 = tree2.root_widget
            if root2:
                dump_tree(root2)
