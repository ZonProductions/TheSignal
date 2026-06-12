"""LIVE combined fix:
1. FloorCulling AdjacentFloorsToShow -> 99 (every re-apply shows everything)
2. Unhide all night-env actors again
3. UDS Sky Light Color Multiplier (Night) -> 3x for interior ambient (persists
   through UDS updates, unlike raw component intensity)"""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'

# 1) neutralize culling
pawn = unreal.GameplayStatics.get_player_pawn(gw, 0)
fixed = False
for c in pawn.get_components_by_class(unreal.ActorComponent):
    if 'FloorCulling' in c.get_class().get_name():
        c.set_editor_property('adjacent_floors_to_show', 99)
        unreal.log(f'FloorCulling: AdjacentFloorsToShow -> '
                   f'{c.get_editor_property("adjacent_floors_to_show")}')
        fixed = True
if not fixed:
    unreal.log_warning('no FloorCullingComponent on pawn!')

# 2) unhide night env
n = 0
for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor):
    lbl = a.get_actor_label()
    if (lbl.startswith(('NightGround', 'Backdrop_'))
            or 'Ultra_Dynamic_Sky' in a.get_class().get_name()):
        if a.get_editor_property('hidden'):
            a.set_actor_hidden_in_game(False)
            n += 1
unreal.log(f'unhid {n} night-env actors')

# 3) night ambient via UDS-native multiplier
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
u.set_editor_property('Sky Light Color Multiplier (Night)',
                      unreal.LinearColor(3.0, 3.0, 3.5, 1.0))
u.call_method('\U0001F4D8Time of Day')
unreal.log('UDS night skylight multiplier -> (3, 3, 3.5) — LIVE')
