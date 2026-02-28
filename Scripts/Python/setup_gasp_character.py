"""
Configure BP_GraceTactical for GASP integration.
Sets up the hidden Mesh (motion-matching) and adds Player mesh.
Run via BlueprintMCP /api/python endpoint.
"""
import unreal

# Load the blueprint
bp_path = '/Game/Player/Blueprints/BP_GraceTactical'
bp = unreal.load_asset(bp_path)
if not bp:
    print('ERROR: Could not load BP_GraceTactical')
else:
    # Get the CDO (Class Default Object) to set inherited component defaults
    bp_class = unreal.load_object(None, bp_path + '.BP_GraceTactical_C')
    cdo = unreal.get_default_object(bp_class)

    # Get the inherited Mesh component
    mesh_comp = cdo.mesh
    if mesh_comp:
        # Set skeletal mesh to SKM_UEFN_Mannequin
        skm = unreal.load_asset('/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin')
        if skm:
            mesh_comp.set_skinned_asset_and_update(skm)
            print(f'Mesh skeletal mesh set to: {skm.get_name()}')
        else:
            print('ERROR: Could not load SKM_UEFN_Mannequin')

        # Set animation class to ABP_SandboxCharacter
        abp = unreal.load_object(None, '/Game/Blueprints/ABP_SandboxCharacter.ABP_SandboxCharacter_C')
        if abp:
            mesh_comp.set_editor_property('anim_class', abp)
            print(f'Mesh anim class set to: ABP_SandboxCharacter')
        else:
            print('ERROR: Could not load ABP_SandboxCharacter')

        # Hide the mesh (it's for motion-matching only)
        mesh_comp.set_editor_property('visible', False)
        print('Mesh visibility set to: False')

        # Always tick pose and refresh bones (needed for motion-matching)
        mesh_comp.set_editor_property('visibility_based_anim_tick_option',
            unreal.EVisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
        print('Mesh tick option set to: AlwaysTickPoseAndRefreshBones')
    else:
        print('ERROR: Could not find inherited Mesh component')

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    print('BP_GraceTactical saved')
