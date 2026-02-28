import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    unreal.log_error('[DEBUG] No PIE world found')
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if not pawn:
        unreal.log_error('[DEBUG] No pawn found')
    else:
        unreal.log(f'[DEBUG] Pawn: {pawn.get_name()} at {pawn.get_actor_location()}')

        capsule_loc = pawn.get_actor_location()
        unreal.log(f'[DEBUG] Capsule WorldLoc: {capsule_loc}')

        ctrl = unreal.GameplayStatics.get_player_controller(world, 0)
        if ctrl:
            cr = ctrl.get_control_rotation()
            unreal.log(f'[DEBUG] ControlRotation: P={cr.pitch:.1f} Y={cr.yaw:.1f} R={cr.roll:.1f}')

        comps = pawn.get_components_by_class(unreal.SceneComponent)
        camera_comp = None
        player_mesh = None
        char_mesh = None

        for c in comps:
            name = c.get_name()
            wl = c.get_world_location()
            rl = c.relative_location

            if 'FirstPersonCamera' in name:
                camera_comp = c
                wr = c.get_world_rotation()
                unreal.log(f'[DEBUG] Camera WorldLoc: X={wl.x:.1f} Y={wl.y:.1f} Z={wl.z:.1f}')
                unreal.log(f'[DEBUG] Camera WorldRot: P={wr.pitch:.1f} Y={wr.yaw:.1f} R={wr.roll:.1f}')
                unreal.log(f'[DEBUG] Camera RelLoc: X={rl.x:.1f} Y={rl.y:.1f} Z={rl.z:.1f}')
                parent = c.get_attach_parent()
                unreal.log(f'[DEBUG] Camera Parent: {parent.get_name() if parent else "NONE"}')

            if name == 'PlayerMesh':
                player_mesh = c
                wr = c.get_world_rotation()
                unreal.log(f'[DEBUG] PlayerMesh WorldLoc: X={wl.x:.1f} Y={wl.y:.1f} Z={wl.z:.1f}')
                unreal.log(f'[DEBUG] PlayerMesh WorldRot: P={wr.pitch:.1f} Y={wr.yaw:.1f} R={wr.roll:.1f}')
                unreal.log(f'[DEBUG] PlayerMesh RelLoc: X={rl.x:.1f} Y={rl.y:.1f} Z={rl.z:.1f}')

            if name == 'CharacterMesh0':
                char_mesh = c
                unreal.log(f'[DEBUG] CharacterMesh0 WorldLoc: X={wl.x:.1f} Y={wl.y:.1f} Z={wl.z:.1f}')
                unreal.log(f'[DEBUG] CharacterMesh0 RelLoc: X={rl.x:.1f} Y={rl.y:.1f} Z={rl.z:.1f}')

        if player_mesh:
            sk = unreal.SkeletalMeshComponent.cast(player_mesh)
            if sk:
                anim = sk.get_anim_instance()
                unreal.log(f'[DEBUG] PlayerMesh AnimInst: {anim.get_class().get_name() if anim else "NONE"}')

                fp_sock = sk.get_socket_bone_name('FPCamera')
                unreal.log(f'[DEBUG] FPCamera socket bone: {fp_sock}')

                fp_world = sk.get_socket_location('FPCamera')
                unreal.log(f'[DEBUG] FPCamera Socket WorldLoc: X={fp_world.x:.1f} Y={fp_world.y:.1f} Z={fp_world.z:.1f}')

                gun_world = sk.get_socket_location('VB ik_hand_gun')
                unreal.log(f'[DEBUG] ik_hand_gun Socket WorldLoc: X={gun_world.x:.1f} Y={gun_world.y:.1f} Z={gun_world.z:.1f}')

                head_world = sk.get_bone_location_by_name('head', unreal.BoneSpaceName.WORLD_SPACE)
                unreal.log(f'[DEBUG] head Bone WorldLoc: X={head_world.x:.1f} Y={head_world.y:.1f} Z={head_world.z:.1f}')

                hand_r = sk.get_bone_location_by_name('hand_r', unreal.BoneSpaceName.WORLD_SPACE)
                unreal.log(f'[DEBUG] hand_r Bone WorldLoc: X={hand_r.x:.1f} Y={hand_r.y:.1f} Z={hand_r.z:.1f}')

        if camera_comp and player_mesh:
            cam_z = camera_comp.get_world_location().z
            sk = unreal.SkeletalMeshComponent.cast(player_mesh)
            if sk:
                fp_z = sk.get_socket_location('FPCamera').z
                gun_z = sk.get_socket_location('VB ik_hand_gun').z
                hand_z = sk.get_bone_location_by_name('hand_r', unreal.BoneSpaceName.WORLD_SPACE).z
                head_z = sk.get_bone_location_by_name('head', unreal.BoneSpaceName.WORLD_SPACE).z
                unreal.log(f'[DEBUG] === HEIGHT COMPARISON (world Z) ===')
                unreal.log(f'[DEBUG]   Camera:      {cam_z:.1f}')
                unreal.log(f'[DEBUG]   FPCamera:    {fp_z:.1f}  (delta from cam: {fp_z - cam_z:.1f})')
                unreal.log(f'[DEBUG]   head bone:   {head_z:.1f}  (delta from cam: {head_z - cam_z:.1f})')
                unreal.log(f'[DEBUG]   hand_r bone: {hand_z:.1f}  (delta from cam: {hand_z - cam_z:.1f})')
                unreal.log(f'[DEBUG]   ik_hand_gun: {gun_z:.1f}  (delta from cam: {gun_z - cam_z:.1f})')

        actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        for a in actors:
            if 'Weapon' in a.get_class().get_name() or 'TR15' in a.get_name():
                wl = a.get_actor_location()
                unreal.log(f'[DEBUG] Weapon Actor: {a.get_name()} at X={wl.x:.1f} Y={wl.y:.1f} Z={wl.z:.1f}')
