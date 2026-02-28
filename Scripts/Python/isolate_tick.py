import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

cam = [c for c in pawn.get_components_by_class(unreal.CameraComponent) if 'FirstPersonCamera' in c.get_name()][0]

ac_fp = None
for c in pawn.get_components_by_class(unreal.ActorComponent):
    if 'AC_FirstPersonCamera' in c.get_class().get_name():
        ac_fp = c
        break

if ac_fp:
    ac_fp.set_component_tick_enabled(False)
    unreal.log(f'[ISOLATE] AC_FirstPersonCamera tick DISABLED')

cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
unreal.log(f'[ISOLATE] Camera RelLoc Z set to 0, waiting for recheck...')
