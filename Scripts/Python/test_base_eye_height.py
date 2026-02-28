import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

unreal.log(f'[TEST] BaseEyeHeight BEFORE: {pawn.base_eye_height}')

cam = None
for c in pawn.get_components_by_class(unreal.CameraComponent):
    if 'FirstPersonCamera' in c.get_name():
        cam = c
        break

unreal.log(f'[TEST] Camera RelLoc Z BEFORE: {cam.relative_location.z:.1f}')

pawn.base_eye_height = 0.0
cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))

unreal.log(f'[TEST] BaseEyeHeight AFTER: {pawn.base_eye_height}')
unreal.log(f'[TEST] Camera RelLoc Z AFTER: {cam.relative_location.z:.1f}')
