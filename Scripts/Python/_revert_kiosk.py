import unreal

# Revert SM_ToiletKiosk and SM_ToiletKioskClosed back to MI_WCPackB
mi_original = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_WCPackB')

for mesh_name in ['SM_ToiletKiosk', 'SM_ToiletKioskClosed']:
    mesh = unreal.load_asset(f'/Game/office_BigCompanyArchViz/StaticMesh/Probs/{mesh_name}')
    if not mesh:
        continue
    static_mats = mesh.get_editor_property('static_materials')
    for sm in static_mats:
        sm.set_editor_property('material_interface', mi_original)
    mesh.set_editor_property('static_materials', static_mats)
    unreal.EditorAssetLibrary.save_asset(f'/Game/office_BigCompanyArchViz/StaticMesh/Probs/{mesh_name}')
    print(f'{mesh_name}: reverted to MI_WCPackB')

# Delete the NoSign MI
unreal.EditorAssetLibrary.delete_asset('/Game/TheSignal/Materials/MI_WCPackB_NoSign')
print('Deleted MI_WCPackB_NoSign')
print('Done.')
