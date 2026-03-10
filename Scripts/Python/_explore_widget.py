import unreal

# Check WidgetLayoutLibrary and WidgetLibrary for tree manipulation
for cls_name in ['WidgetLayoutLibrary', 'WidgetLibrary']:
    cls = getattr(unreal, cls_name, None)
    if cls:
        methods = [x for x in dir(cls) if not x.startswith('_')]
        print(f'{cls_name}: {methods}')
        print()

# Check if UMG objects are constructable
for cls_name in ['VerticalBox', 'HorizontalBox', 'WidgetSwitcher', 'Button', 'TextBlock', 'CanvasPanel', 'Overlay', 'ScrollBox']:
    try:
        cls = getattr(unreal, cls_name, None)
        if cls:
            print(f'{cls_name}: EXISTS')
        else:
            print(f'{cls_name}: NOT FOUND')
    except:
        print(f'{cls_name}: ERROR')

# Can we access widget tree via low-level FProperty iteration?
print()
wb = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
# Try brute-force property access
for prop_name in ['WidgetTree', 'widget_tree', 'widgettree', 'widget_blueprint_generated_class']:
    try:
        val = wb.get_editor_property(prop_name)
        print(f'  FOUND: {prop_name} = {val}')
    except:
        pass

# Check if WidgetTree is a UObject subclass we can find
for name in dir(unreal):
    if 'WidgetTree' in name:
        print(f'Found class: {name}')
