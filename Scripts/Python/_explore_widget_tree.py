import unreal

# Load WBP_InventoryMenu_Horror
wb = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
print(f'Type: {type(wb)}')
print(f'Class: {wb.get_class().get_name()}')

# Check for widget_tree access
for prop_name in ['widget_tree', 'WidgetTree', 'widgettree']:
    try:
        val = wb.get_editor_property(prop_name)
        print(f'FOUND: {prop_name} = {val} (type: {type(val)})')
    except Exception as e:
        print(f'FAIL: {prop_name} -> {e}')

# Check if WidgetTree class exists in unreal module
for name in dir(unreal):
    if 'widget' in name.lower() and 'tree' in name.lower():
        print(f'Class found: {name}')

# Check all properties on the widget blueprint
print('\n--- All properties with widget/tree/canvas/root ---')
for attr in dir(wb):
    lower = attr.lower()
    if any(kw in lower for kw in ['widget', 'tree', 'canvas', 'root', 'hierarchy']):
        print(f'  {attr}')

# Try to get the generated class and check its properties
gen_class = wb.get_editor_property('generated_class')
if gen_class:
    print(f'\nGenerated class: {gen_class.get_name()}')
    cdo = unreal.get_default_object(gen_class)
    if cdo:
        print(f'CDO type: {type(cdo)}')
        for attr in dir(cdo):
            lower = attr.lower()
            if any(kw in lower for kw in ['widget', 'tree', 'canvas', 'root']):
                print(f'  CDO.{attr}')
