"""Set the default skeletal mesh and anim class on BP_GraceTactical's Player component."""
import unreal

bp = unreal.load_asset('/Game/Player/Blueprints/BP_GraceTactical')
scs = None

# Access SCS nodes through the blueprint's generated class
bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)

# Try to find Player component through all components
all_comps = cdo.get_components_by_class(unreal.SkeletalMeshComponent)
print(f'Found {len(all_comps)} SkeletalMeshComponent(s)')
for comp in all_comps:
    name = comp.get_name()
    print(f'  Component: {name}')
    if 'Player' in name:
        # Set the skeletal mesh
        skm = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
        if skm:
            comp.set_skinned_asset_and_update(skm)
            print(f'  -> Skeletal mesh set to: {skm.get_name()}')

        # Set the anim class
        abp = unreal.load_object(None, '/Game/KINEMATION/TacticalShooterPack/Animations/UE5/ABP_TacticalShooter_UE5.ABP_TacticalShooter_UE5_C')
        if abp:
            comp.set_editor_property('anim_class', abp)
            print(f'  -> Anim class set to: ABP_TacticalShooter_UE5')

        # Verify
        current_mesh = comp.get_editor_property('skinned_asset')
        current_anim = comp.get_editor_property('anim_class')
        print(f'  Verify mesh: {current_mesh}')
        print(f'  Verify anim: {current_anim}')

# Save
unreal.EditorAssetLibrary.save_loaded_asset(bp)
print('Saved BP_GraceTactical')
