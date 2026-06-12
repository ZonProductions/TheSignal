"""LIVE-ONLY: flip Occlusion Mode -> Off on the settings object the PIE UDS
is using (in-memory, nothing saved to disk). Fires UDS update."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
os_ = u.get_editor_property('Occlusion Settings')
mode_cls = type(os_.get_editor_property('Occlusion Mode'))
unreal.log(f'enum entries: {[str(v) for v in mode_cls]}')
before = os_.get_editor_property('Occlusion Mode')
os_.set_editor_property('Occlusion Mode', mode_cls.TEST_IF_INSIDE_OCCLUSION_VOLUME)
os_.set_editor_property('Global Occlusion Min Fraction', 0.0)
os_.set_editor_property('Global Occlusion Max Fraction', 0.0)
u.call_method('\U0001F4D8Time of Day')
unreal.log(f'PIE LIVE: Occlusion Mode {before} -> '
           f'{os_.get_editor_property("Occlusion Mode")}, fractions=0 (in-memory only)')

# confirm no occlusion volumes exist that could still trigger volume mode
n_vol = sum(1 for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
            if 'Occlusion' in a.get_class().get_name() and 'Volume' in a.get_class().get_name())
unreal.log(f'occlusion volumes in level: {n_vol}')
