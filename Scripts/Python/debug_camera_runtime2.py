import unreal

# Get game world via subsystem
ess = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ess.get_game_world()
if not world:
    print("ERROR: No game world - is PIE running?")
else:
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if not pc:
        print("ERROR: No player controller")
    else:
        pawn = pc.get_pawn()
        print(f"Pawn: {pawn.get_class().get_name() if pawn else 'NONE'}")
        
        if pawn:
            # Get all skeletal mesh components
            sk_comps = pawn.get_components_by_class(unreal.SkeletalMeshComponent)
            for sk in sk_comps:
                name = sk.get_name()
                mesh = sk.get_editor_property('skeletal_mesh_asset')
                anim = sk.get_anim_instance()
                rel_loc = sk.get_editor_property('relative_location')
                rel_rot = sk.get_editor_property('relative_rotation')
                world_loc = sk.get_world_location()
                world_rot = sk.get_world_rotation()
                parent = sk.get_attach_parent()
                print(f'\nSkeletalMesh "{name}":')
                print(f'  Mesh: {mesh.get_name() if mesh else "NONE"}')
                print(f'  AnimInstance: {anim.get_class().get_name() if anim else "NONE"}')
                print(f'  Parent: {parent.get_name() if parent else "NONE"}')
                print(f'  RelLoc: {rel_loc}')
                print(f'  RelRot: {rel_rot}')
                print(f'  WorldLoc: {world_loc}')
                print(f'  WorldRot: {world_rot}')
                
                if sk.does_socket_exist('FPCamera'):
                    t = sk.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_WORLD)
                    print(f'  FPCamera WORLD: Loc={t.translation}, Rot={t.rotation.rotator()}')
                    t2 = sk.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_COMPONENT)
                    print(f'  FPCamera COMP: Loc={t2.translation}, Rot={t2.rotation.rotator()}')
            
            # Get camera component
            cam_comps = pawn.get_components_by_class(unreal.CameraComponent)
            for cam in cam_comps:
                name = cam.get_name()
                parent = cam.get_attach_parent()
                socket = cam.get_attach_socket_name()
                world_loc = cam.get_world_location()
                world_rot = cam.get_world_rotation()
                rel_loc = cam.get_editor_property('relative_location')
                rel_rot = cam.get_editor_property('relative_rotation')
                use_pawn = cam.get_editor_property('b_use_pawn_control_rotation')
                print(f'\nCamera "{name}":')
                print(f'  Parent: {parent.get_name() if parent else "NONE"}')
                print(f'  Socket: {socket}')
                print(f'  bUsePawnControlRotation: {use_pawn}')
                print(f'  RelLoc: {rel_loc}')
                print(f'  RelRot: {rel_rot}')
                print(f'  WorldLoc: {world_loc}')
                print(f'  WorldRot: {world_rot}')
            
            # Check AC_FirstPersonCamera state
            all_comps = pawn.get_components_by_class(unreal.ActorComponent)
            for comp in all_comps:
                cname = comp.get_class().get_name()
                if 'FirstPersonCamera' in cname:
                    print(f'\n--- AC_FirstPersonCamera ---')
                    for prop_name in ['Camera', 'Mesh', 'OwnerAnimInstance', 'OwnerCharacter']:
                        try:
                            val = comp.get_editor_property(prop_name)
                            if val:
                                print(f'  {prop_name}: {val.get_name()} ({val.get_class().get_name()})')
                            else:
                                print(f'  {prop_name}: None')
                        except Exception as e:
                            print(f'  {prop_name}: ERROR - {e}')
            
            # Control rotation
            cr = pc.get_control_rotation()
            print(f'\nControlRotation: pitch={cr.pitch}, yaw={cr.yaw}, roll={cr.roll}')
            print(f'Pawn Location: {pawn.get_actor_location()}')
            print(f'Pawn Rotation: {pawn.get_actor_rotation()}')
