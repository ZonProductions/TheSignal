"""LIVE: unhide the night environment that floor culling wrongly hid.
Temporary — culling re-applies when player changes Z bucket."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
n = 0
for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor):
    lbl = a.get_actor_label()
    if (lbl.startswith(('NightGround', 'Backdrop_'))
            or 'Ultra_Dynamic_Sky' in a.get_class().get_name()):
        if a.get_editor_property('hidden'):
            a.set_actor_hidden_in_game(False)
            n += 1
unreal.log(f'unhid {n} night-env actors (LIVE — until next floor-bucket change)')
