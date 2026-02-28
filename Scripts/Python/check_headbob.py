"""Check bUseBuiltInHeadBob and set PeekAction on BP_GraceCharacter."""
import unreal

eal = unreal.EditorAssetLibrary

# Check head bob setting
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
gc = bp.generated_class()
cdo = unreal.get_default_object(gc)

for c in cdo.get_components_by_class(unreal.ActorComponent):
    if 'GameplayComp' in c.get_name():
        val = c.get_editor_property('bUseBuiltInHeadBob')
        unreal.log(f'bUseBuiltInHeadBob = {val}')
        if not val:
            c.set_editor_property('bUseBuiltInHeadBob', True)
            unreal.log('SET bUseBuiltInHeadBob = True')
        break

# Set PeekAction CDO property
ia_peek = unreal.load_asset('/Game/Core/Input/Actions/IA_Peek')
if ia_peek:
    cdo.set_editor_property('PeekAction', ia_peek)
    eal.save_asset('/Game/Core/Player/BP_GraceCharacter')
    unreal.log(f'PeekAction set to: {cdo.get_editor_property("PeekAction")}')
else:
    unreal.log_warning('IA_Peek not found!')

unreal.log('Done')
