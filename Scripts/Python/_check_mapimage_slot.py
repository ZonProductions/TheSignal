"""Check MapImage's slot properties in WBP_InventoryMenu_Horror."""
import unreal

wbp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
if not wbp:
    print('WBP not found')
else:
    print(f'WBP type: {wbp.get_class().get_name()}')
    # Try to access widget tree through the blueprint's generated class
    tree = wbp.get_editor_property('widget_tree')
    if tree:
        print(f'WidgetTree: {tree}')
        map_img = tree.find_widget(unreal.Name('MapImage'))
        if map_img:
            print(f'MapImage: {map_img.get_name()} ({map_img.get_class().get_name()})')
            slot = map_img.slot
            if slot:
                print(f'Slot class: {slot.get_class().get_name()}')
                for p in dir(slot):
                    if not p.startswith('_'):
                        try:
                            v = getattr(slot, p)
                            if not callable(v):
                                print(f'  slot.{p} = {v}')
                        except:
                            pass
            parent = map_img.get_parent()
            if parent:
                print(f'Parent: {parent.get_name()} ({parent.get_class().get_name()})')
        else:
            print('MapImage not found in tree')
            # List widgets
            root = tree.root_widget
            if root:
                print(f'Root: {root.get_name()} ({root.get_class().get_name()})')
    else:
        print('No widget tree')
