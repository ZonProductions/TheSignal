import unreal

for path in ['/Game/Core/Items/DA_ExplosiveGrenade',
             '/Game/InventorySystemPro/ExampleContent/Common/Items/Weapons/DA_ExplosiveGrenade']:
    da = unreal.load_asset(path)
    if not da:
        unreal.log_warning(f'{path}: NOT FOUND')
        continue
    old = da.get_editor_property('MaxStackAmount')
    da.set_editor_property('MaxStackAmount', 5)
    saved = unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(f'{path}: MaxStackAmount {old} -> 5, saved={saved}')
