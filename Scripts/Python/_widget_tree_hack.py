import unreal

# Load WBP_InventoryMenu_Horror
wb = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
print(f'Asset: {wb}')

# Try 1: Direct attribute access (bypasses get_editor_property protection)
try:
    wt = getattr(wb, 'widget_tree', None)
    if wt:
        print(f'getattr widget_tree: {wt} (type: {type(wt)})')
    else:
        print('getattr widget_tree: None')
except Exception as e:
    print(f'getattr widget_tree error: {e}')

# Try 2: Check if WidgetTree class exists
wt_cls = getattr(unreal, 'WidgetTree', None)
print(f'unreal.WidgetTree: {wt_cls}')

# Try 3: Check UWidgetBlueprintLibrary for anything useful
wbl = getattr(unreal, 'WidgetBlueprintLibrary', None)
if wbl:
    methods = [x for x in dir(wbl) if not x.startswith('_')]
    print(f'WidgetBlueprintLibrary methods: {methods}')

# Try 4: EditorUtilityLibrary
eul = getattr(unreal, 'EditorUtilityLibrary', None)
if eul:
    methods = [x for x in dir(eul) if not x.startswith('_') and 'widget' in x.lower()]
    print(f'EditorUtilityLibrary widget methods: {methods}')

# Try 5: Check WidgetBlueprintFactory for modification APIs
wbf = getattr(unreal, 'WidgetBlueprintFactory', None)
if wbf:
    methods = [x for x in dir(wbf) if not x.startswith('_')]
    print(f'WidgetBlueprintFactory methods: {methods}')

# Try 6: Check all unreal classes with "Widget" in the name
print('\n--- Widget-related classes ---')
for name in sorted(dir(unreal)):
    if 'Widget' in name and not name.startswith('_'):
        cls = getattr(unreal, name)
        if isinstance(cls, type):
            # Check if it has tree/root/add methods
            relevant = [m for m in dir(cls) if any(kw in m.lower() for kw in ['tree', 'root', 'add_child', 'construct', 'hierarchy'])]
            if relevant:
                print(f'  {name}: {relevant}')

# Try 7: Check if we can access the Blueprint's generated class CDO
# and from there get the WidgetTree
try:
    gen_class_name = wb.get_class().get_name()
    print(f'\nBlueprint class name: {gen_class_name}')
except Exception as e:
    print(f'get_class error: {e}')

# Try 8: Use unreal.find_object or load_object to find the WidgetTree
# WidgetTree is a subobject of the WidgetBlueprint
asset_path = '/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror'
for suffix in ['.WidgetTree', ':WidgetTree', '.WidgetTree_0']:
    try:
        obj = unreal.load_object(name=asset_path + suffix, outer=None)
        if obj:
            print(f'Found: {asset_path}{suffix} -> {obj} (type: {type(obj)})')
    except Exception as e:
        pass

# Try 9: Find all subobjects of the widget blueprint
try:
    # Use EditorAssetLibrary to list subobjects
    eal = unreal.EditorAssetLibrary
    metadata = eal.get_metadata_tag(asset_path, 'WidgetTree')
    print(f'Metadata WidgetTree: {metadata}')
except Exception as e:
    pass

# Try 10: WidgetBlueprintEditorUtils or similar
for name in dir(unreal):
    if 'blueprint' in name.lower() and 'editor' in name.lower():
        print(f'Editor class: {name}')
    if 'widget' in name.lower() and ('edit' in name.lower() or 'util' in name.lower()):
        print(f'Widget util: {name}')
