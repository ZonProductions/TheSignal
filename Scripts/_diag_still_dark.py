"""State check: did unhide stick? skylight intensity? pawn bucket? + screenshot."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ues.get_game_world()
assert gw, 'PIE not running'
pawn = unreal.GameplayStatics.get_player_pawn(gw, 0)
z = pawn.get_actor_location().z
unreal.log(f'pawn Z={z:.0f} bucket={int(z // 500)}')

for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor):
    lbl = a.get_actor_label()
    cls = a.get_class().get_name()
    if lbl in ('NightGround', 'Backdrop_ConcreteSlab') or 'Ultra_Dynamic_Sky' in cls:
        unreal.log(f'{lbl} hidden={a.get_editor_property("hidden")}')
    if 'Ultra_Dynamic_Sky' in cls:
        for c in a.get_components_by_class(unreal.SkyLightComponent):
            if c.get_editor_property('visible'):
                unreal.log(f'skylight intensity={c.get_editor_property("intensity")} '
                           f'cast_shadows={c.get_editor_property("cast_shadows")} '
                           f'real_time_capture={c.get_editor_property("real_time_capture")}')
        moon = a.get_editor_property('Moon')
        unreal.log(f'moon intensity={moon.get_editor_property("intensity")} '
                   f'visible={moon.get_editor_property("visible")}')

unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, 'still_dark_view.png')
unreal.log('screenshot queued')
