import unreal

# Change the default material on the SM_ToiletKiosk and SM_ToiletKioskClosed
# mesh assets directly (not per-instance).

mi_nosign = unreal.load_asset('/Game/TheSignal/Materials/MI_WCPackB_NoSign')
if not mi_nosign:
    print('ERROR: MI_WCPackB_NoSign not found')
else:
    for mesh_name in ['SM_ToiletKiosk', 'SM_ToiletKioskClosed']:
        mesh = unreal.load_asset(f'/Game/office_BigCompanyArchViz/StaticMesh/Probs/{mesh_name}')
        if not mesh:
            print(f'  {mesh_name}: NOT FOUND')
            continue

        static_mats = mesh.get_editor_property('static_materials')
        print(f'{mesh_name}: {len(static_mats)} material slots')
        for i, sm in enumerate(static_mats):
            old_mat = sm.get_editor_property('material_interface')
            old_name = old_mat.get_name() if old_mat else 'None'
            print(f'  Slot {i}: {old_name}')
            sm.set_editor_property('material_interface', mi_nosign)
            print(f'  Slot {i}: → MI_WCPackB_NoSign')

        mesh.set_editor_property('static_materials', static_mats)
        unreal.EditorAssetLibrary.save_asset(f'/Game/office_BigCompanyArchViz/StaticMesh/Probs/{mesh_name}')
        print(f'  Saved {mesh_name}')

    print('Done. All kiosk instances should update.')
