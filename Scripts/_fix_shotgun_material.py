import unreal

sm = unreal.load_asset('/Game/Core/Meshes/SM_PoliceShotgun')
mi = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Weapons/Herrington_11-87/Materials/MI_Herrington_11-87_Police')
assert sm and mi, 'mesh or material missing'

mats = sm.get_editor_property('static_materials')
for m in mats:
    m.set_editor_property('material_interface', mi)
sm.set_editor_property('static_materials', mats)

ok = unreal.EditorAssetLibrary.save_asset('/Game/Core/Meshes/SM_PoliceShotgun')
if not ok:
    ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f'SM_PoliceShotgun now uses {mi.get_name()}, saved={ok}')
