"""Confirm floor culling is the blackbox: hidden state of UDS/ground/backdrop
in live PIE + pawn Z + bucket math."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
pawn = unreal.GameplayStatics.get_player_pawn(gw, 0)
z = pawn.get_actor_location().z
unreal.log(f'pawn Z={z:.0f} -> bucket={int((z - 0) // 500)}')

for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor):
    lbl = a.get_actor_label()
    if (lbl in ('NightGround', 'NightGround2', 'Backdrop_ConcreteSlab')
            or 'Ultra_Dynamic_Sky' in a.get_class().get_name()
            or lbl.startswith('Backdrop_Hill_0')):
        unreal.log(f'{lbl} Z={a.get_actor_location().z:.0f} '
                   f'hidden_in_game={a.get_editor_property("hidden")}')
