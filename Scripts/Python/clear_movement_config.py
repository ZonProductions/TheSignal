"""Clear MovementConfig DataAsset from BP_GraceCharacter.
C++ fallback defaults become the source of truth."""
import unreal

bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
gc = bp.generated_class()
cdo = unreal.get_default_object(gc)
cdo.set_editor_property('MovementConfig', None)
unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
unreal.log('MovementConfig cleared — C++ defaults are now source of truth')
