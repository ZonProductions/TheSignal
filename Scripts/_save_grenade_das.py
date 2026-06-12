import unreal

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f'save_dirty_packages returned: {ok}')
for path in ['/Game/Core/Items/DA_ExplosiveGrenade',
             '/Game/InventorySystemPro/ExampleContent/Common/Items/Weapons/DA_ExplosiveGrenade']:
    da = unreal.load_asset(path)
    unreal.log(f'{path}: MaxStackAmount={da.get_editor_property("MaxStackAmount")}, dirty={unreal.EditorAssetLibrary.save_asset(path)}')
