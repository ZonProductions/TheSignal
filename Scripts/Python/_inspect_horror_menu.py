import unreal

bp = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Horror/UI/Menus/WBP_InventoryMenu_Horror')
gen_class = bp.generated_class()
cdo = unreal.get_default_object(gen_class)
print('CDO type:', type(cdo))

# List all properties that might be widget refs
for attr in dir(cdo):
    lower = attr.lower()
    if any(kw in lower for kw in ['tab', 'map', 'header', 'canvas', 'grid', 'shortcut', 'image', 'menu', 'panel', 'note']):
        try:
            val = getattr(cdo, attr, None)
            if val is not None and not callable(val):
                print(f'  {attr} = {val} ({type(val).__name__})')
            elif not callable(val):
                print(f'  {attr} = None')
        except:
            print(f'  {attr} = <error reading>')
