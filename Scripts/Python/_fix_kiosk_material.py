import unreal

# Create a new MI based on M_ProbBase that uses the same look as MI_WCPackB
# but without the toilet sign texture. We'll duplicate MI_WCPackB and replace
# just the Color texture with a solid color.

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

mi_path = '/Game/TheSignal/Materials/MI_WCPackB_NoSign'
mi = unreal.load_asset(mi_path)

if not mi:
    # Duplicate MI_WCPackB as our base
    src = '/Game/office_BigCompanyArchViz/Materials/MI_WCPackB'
    eal.duplicate_asset(src, mi_path)
    mi = unreal.load_asset(mi_path)
    print(f'Duplicated MI_WCPackB → {mi_path}')

if not mi:
    print('ERROR: Could not create material instance')
else:
    # The toilet sign is in T_WCPackB_BaseColor texture.
    # Replace it with T_WCPackA_BaseColor which is the same stall material
    # but for the other pack variant — may not have a toilet sign.
    # Or use a plain solid texture.

    # Check what T_WCPackA_BaseColor looks like
    tex_a = unreal.load_asset('/Game/office_BigCompanyArchViz/Textures/T_WCPackA_BaseColor')
    tex_b = unreal.load_asset('/Game/office_BigCompanyArchViz/Textures/T_WCPackB_BaseColor')
    print(f'T_WCPackA_BaseColor: {tex_a}')
    print(f'T_WCPackB_BaseColor: {tex_b}')

    # Try WCPackA texture — if it doesn't have the sign, use it
    if tex_a:
        mi.set_editor_property('texture_parameter_values', mi.get_editor_property('texture_parameter_values'))
        # Set Color param to WCPackA texture
        tex_params = mi.get_editor_property('texture_parameter_values')
        for tp in tex_params:
            pname = tp.get_editor_property('parameter_info').get_editor_property('name')
            if pname == 'Color':
                tp.set_editor_property('parameter_value', tex_a)
                print(f'  Swapped Color texture to T_WCPackA_BaseColor')
        mi.set_editor_property('texture_parameter_values', tex_params)
        eal.save_asset(mi_path)
        print(f'  Saved {mi_path}')

    # Now apply to all kiosk actors
    subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    all_actors = subsys.get_all_level_actors()
    count = 0
    for a in all_actors:
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        smc = a.static_mesh_component
        if not smc or not smc.static_mesh:
            continue
        mesh_name = smc.static_mesh.get_name()
        if mesh_name in ('SM_ToiletKiosk', 'SM_ToiletKioskClosed'):
            smc.set_material(0, mi)
            print(f'  {a.get_actor_label()}: → MI_WCPackB_NoSign')
            count += 1

    print(f'\nSwapped {count} kiosk stall doors')
    if count > 0:
        subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        subsys_level.save_all_dirty_levels()
        print('Level saved.')
