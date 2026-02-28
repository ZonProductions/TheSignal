"""Fix PlayerMesh rotation: pitch=-90 (wrong) -> yaw=-90 (correct)."""
import unreal

bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
pm = cdo.get_editor_property('PlayerMesh')
pm.set_editor_property('relative_rotation', unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
rot = pm.get_editor_property('relative_rotation')
unreal.log(f'[FIX] PlayerMesh rotation: {rot}')
