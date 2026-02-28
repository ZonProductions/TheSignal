import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

ctrl = pawn.get_controller()
ctrl_rot = ctrl.get_control_rotation() if ctrl else None

player_mesh = None
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    if 'PlayerMesh' in c.get_name():
        player_mesh = c
        break

cam = None
for c in pawn.get_components_by_class(unreal.CameraComponent):
    if 'FirstPersonCamera' in c.get_name():
        cam = c
        break

unreal.log(f'[DEBUG] ControlRotation: P={ctrl_rot.pitch:.1f} Y={ctrl_rot.yaw:.1f} R={ctrl_rot.roll:.1f}')
unreal.log(f'[DEBUG] Camera WorldRot: P={cam.get_world_rotation().pitch:.1f} Y={cam.get_world_rotation().yaw:.1f}')
unreal.log(f'[DEBUG] Camera WorldLoc: Z={cam.get_world_location().z:.1f}')

fp_loc = player_mesh.get_socket_location('FPCamera')
unreal.log(f'[DEBUG] FPCamera Socket Loc: X={fp_loc.x:.1f} Y={fp_loc.y:.1f} Z={fp_loc.z:.1f}')

gun_loc = player_mesh.get_socket_location('ik_hand_gun')
unreal.log(f'[DEBUG] ik_hand_gun Loc: X={gun_loc.x:.1f} Y={gun_loc.y:.1f} Z={gun_loc.z:.1f}')

spine_loc = player_mesh.get_socket_location('spine_02')
unreal.log(f'[DEBUG] spine_02 Loc: X={spine_loc.x:.1f} Y={spine_loc.y:.1f} Z={spine_loc.z:.1f}')

anim = player_mesh.get_anim_instance()
pitch_val = anim.get_editor_property('Pitch')
unreal.log(f'[DEBUG] AnimBP Pitch = {pitch_val:.1f}')
unreal.log(f'[DEBUG] Delta camera-gun Z = {cam.get_world_location().z - gun_loc.z:.1f}')
