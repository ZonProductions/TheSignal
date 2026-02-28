import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)

if not pawn:
    unreal.log_error('[DEBUG] No pawn found')
else:
    ctrl = pawn.get_controller()
    ctrl_rot = ctrl.get_control_rotation() if ctrl else None
    actor_rot = pawn.get_actor_rotation()
    unreal.log(f'[DEBUG] ControlRot: P={ctrl_rot.pitch:.1f} Y={ctrl_rot.yaw:.1f}' if ctrl_rot else '[DEBUG] No controller')
    unreal.log(f'[DEBUG] ActorRot: P={actor_rot.pitch:.1f} Y={actor_rot.yaw:.1f}')

    player_mesh = None
    for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
        if 'PlayerMesh' in c.get_name():
            player_mesh = c
            break

    if player_mesh:
        anim = player_mesh.get_anim_instance()
        unreal.log(f'[DEBUG] PlayerMesh AnimInstance: {anim.get_class().get_name() if anim else "NULL"}')
        if anim:
            try:
                pitch_val = anim.get_editor_property('Pitch')
                unreal.log(f'[DEBUG] AnimBP Pitch = {pitch_val}')
            except:
                unreal.log_warning('[DEBUG] Could not read Pitch from AnimBP')
            try:
                char_val = anim.get_editor_property('Character')
                unreal.log(f'[DEBUG] AnimBP Character = {char_val.get_name() if char_val else "NULL"}')
            except:
                unreal.log_warning('[DEBUG] Could not read Character from AnimBP')
            try:
                sc = anim.get_editor_property('ShooterComponent')
                unreal.log(f'[DEBUG] AnimBP ShooterComponent = {sc.get_class().get_name() if sc else "NULL"}')
            except:
                unreal.log_warning('[DEBUG] Could not read ShooterComponent from AnimBP')
    else:
        unreal.log_error('[DEBUG] PlayerMesh not found')

    inherited_mesh = pawn.mesh
    if inherited_mesh:
        ianim = inherited_mesh.get_anim_instance()
        unreal.log(f'[DEBUG] InheritedMesh: {inherited_mesh.get_name()}, SKM={inherited_mesh.skeletal_mesh_asset}, AnimInst={ianim.get_class().get_name() if ianim else "NULL"}')
    else:
        unreal.log('[DEBUG] No inherited mesh')
