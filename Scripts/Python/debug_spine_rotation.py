import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

player_mesh = None
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    if 'PlayerMesh' in c.get_name():
        player_mesh = c
        break

anim = player_mesh.get_anim_instance()
pitch = anim.get_editor_property('Pitch')
ctrl = pawn.get_controller()
ctrl_rot = ctrl.get_control_rotation()

unreal.log(f'[SPINE] ControlRot Pitch: {ctrl_rot.pitch:.1f} (wrapped: {ctrl_rot.pitch - 360 if ctrl_rot.pitch > 180 else ctrl_rot.pitch:.1f})')
unreal.log(f'[SPINE] AnimBP Pitch: {pitch:.1f}')

bones = ['spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'head', 'neck_01']
for bone in bones:
    idx = player_mesh.get_bone_index(bone)
    if idx >= 0:
        loc = player_mesh.get_socket_location(bone)
        unreal.log(f'[SPINE] {bone}: Z={loc.z:.1f}')
    else:
        unreal.log(f'[SPINE] {bone}: NOT FOUND')

fp_loc = player_mesh.get_socket_location('FPCamera')
gun_loc = player_mesh.get_socket_location('ik_hand_gun')
cam_loc = [c for c in pawn.get_components_by_class(unreal.CameraComponent) if 'FirstPersonCamera' in c.get_name()][0].get_world_location()
unreal.log(f'[SPINE] FPCamera Z={fp_loc.z:.1f}, gun Z={gun_loc.z:.1f}, camera Z={cam_loc.z:.1f}')
