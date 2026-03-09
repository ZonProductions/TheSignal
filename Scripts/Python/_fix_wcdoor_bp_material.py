import unreal

# Swap the material override on BP_WCDoor01 and BP_WCDoor02 Blueprint CDOs.
# Change the OverrideMaterials on the SM_DoorOffice component template.

new_mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_DoorBW')
if not new_mat:
    print('ERROR: MI_DoorBW not found!')
else:
    print(f'Target material: {new_mat.get_name()}')

    for bp_name in ['BP_WCDoor01', 'BP_WCDoor02']:
        bp = unreal.load_asset(f'/Game/office_BigCompanyArchViz/Blueprints/{bp_name}')
        if not bp:
            print(f'  {bp_name}: NOT FOUND')
            continue

        # Get the CDO (default object)
        cdo = unreal.get_default_object(bp.generated_class())
        if not cdo:
            print(f'  {bp_name}: Could not get CDO')
            continue

        # Find SM_DoorOffice component on the CDO
        smcs = cdo.get_components_by_class(unreal.StaticMeshComponent)
        for smc in smcs:
            if 'DoorOffice' in smc.get_name() or 'Door' in smc.get_name():
                old_mat = smc.get_material(0)
                old_name = old_mat.get_name() if old_mat else 'None'
                smc.set_material(0, new_mat)
                print(f'  {bp_name}: {smc.get_name()} — {old_name} → MI_DoorBW')
                break

        # Save the Blueprint asset
        unreal.EditorAssetLibrary.save_asset(f'/Game/office_BigCompanyArchViz/Blueprints/{bp_name}')
        print(f'  {bp_name}: saved')

    print('Done. Existing instances should update on next editor refresh.')
