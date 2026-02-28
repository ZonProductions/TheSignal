"""Check DA_TacticalCharacterSettings - find default mesh assets."""
import unreal

# Load an instance of the DA (not the class, an actual data asset)
# CharacterSettings on BP_GraceTactical CDO should point to an instance
bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)
cs = cdo.get_editor_property('CharacterSettings')
print(f'CharacterSettings: {cs}')
if cs:
    print(f'  type: {type(cs).__name__}')
    print(f'  name: {cs.get_name()}')
    print(f'  path: {cs.get_path_name()}')

# Check the original BP_TacticalShooterCharacter to see what mesh it uses
orig_class = unreal.load_object(None, '/Game/KINEMATION/TacticalShooterPack/Blueprints/BP_TacticalShooterCharacter.BP_TacticalShooterCharacter_C')
orig_cdo = unreal.get_default_object(orig_class)
orig_mesh = orig_cdo.mesh
if orig_mesh:
    sa = orig_mesh.get_editor_property('skeletal_mesh_asset')
    if sa is None:
        sa = orig_mesh.get_editor_property('skinned_asset')
    print(f'Original Mesh skeletal_asset: {sa}')
    anim = orig_mesh.get_editor_property('anim_class')
    print(f'Original Mesh anim_class: {anim}')
