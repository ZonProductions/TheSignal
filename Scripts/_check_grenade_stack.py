import unreal

for path in ['/Game/Core/Items/DA_ExplosiveGrenade',
             '/Game/InventorySystemPro/ExampleContent/Common/Items/Weapons/DA_ExplosiveGrenade']:
    da = unreal.load_asset(path)
    if not da:
        unreal.log(f'{path}: NOT FOUND')
        continue
    unreal.log(f'=== {path} ({da.get_class().get_name()})')
    # Dump all editable properties whose name smells like stacking/quantity
    for prop in ('MaxStackSize', 'StackSize', 'MaxStack', 'bStackable', 'Stackable',
                 'MaxQuantity', 'bIsStackable', 'MaxAmount', 'AmountPerStack'):
        try:
            unreal.log(f'  {prop} = {da.get_editor_property(prop)}')
        except Exception:
            pass
