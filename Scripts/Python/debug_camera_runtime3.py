import unreal

ess = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ess.get_game_world()
if not world:
    print("ERROR: No game world - is PIE running?")
else:
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if not pc:
        print("ERROR: No player controller")
    else:
        pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
        print(f"Pawn: {pawn.get_class().get_name() if pawn else 'NONE'}")
        
        if pawn:
            # Skeletal meshes
            sk_comps = pawn.get_components_by_class(unreal.SkeletalMeshComponent)
            for sk in sk_comps:
                name = sk.get_name()
                mesh_asset = sk.get_editor_property('skeletal_mesh_asset')
                anim = sk.get_anim_instance()
                parent = sk.get_attach_parent()
                world_loc = sk.get_world_location()
                world_rot = sk.get_world_rotation()
                print(f'\nSkeletalMesh "{name}":')
                print(f'  Asset: {mesh_asset.get_name() if mesh_asset else "NONE"}')
                print(f'  AnimInst: {anim.get_class().get_name() if anim else "NONE"}')
                print(f'  Parent: {parent.get_name() if parent else "NONE"}')
                print(f'  WorldLoc: {world_loc}')
                print(f'  WorldRot: {world_rot}')
                
                if sk.does_socket_exist('FPCamera'):
                    tw = sk.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_WORLD)
                    tc = sk.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_COMPONENT)
                    print(f'  FPCamera WORLD: Loc={tw.translation}')
                    print(f'  FPCamera WORLD: Rot={tw.rotation.rotator()}')
                    print(f'  FPCamera COMP:  Loc={tc.translation}')
            
            # Camera
            cam_comps = pawn.get_components_by_class(unreal.CameraComponent)
            for cam in cam_comps:
                parent = cam.get_attach_parent()
                socket = cam.get_attach_socket_name()
                print(f'\nCamera "{cam.get_name()}":')
                print(f'  Parent: {parent.get_name() if parent else "NONE"}')
                print(f'  Socket: {socket}')
                print(f'  bUsePawnControlRotation: {cam.get_editor_property("b_use_pawn_control_rotation")}')
                print(f'  WorldLoc: {cam.get_world_location()}')
                print(f'  WorldRot: {cam.get_world_rotation()}')
            
            # AC_FirstPersonCamera properties
            all_comps = pawn.get_components_by_class(unreal.ActorComponent)
            for comp in all_comps:
                cname = comp.get_class().get_name()
                if 'FirstPersonCamera' in cname:
                    print(f'\n--- {cname} ---')
                    for pn in ['Camera', 'Mesh', 'OwnerAnimInstance', 'OwnerCharacter']:
                        try:
                            val = comp.get_editor_property(pn)
                            print(f'  {pn}: {val.get_name() + " (" + val.get_class().get_name() + ")" if val else "None"}')
                        except Exception as e:
                            print(f'  {pn}: ERR({e})')
                elif 'TacticalShooterAnimation' in cname:
                    print(f'\n--- {cname} ---')
                    for pn in ['ActiveSettings', 'IsAiming', 'Gait']:
                        try:
                            val = comp.get_editor_property(pn)
                            print(f'  {pn}: {val}')
                        except Exception as e:
                            print(f'  {pn}: ERR({e})')
            
            cr = pc.get_control_rotation()
            print(f'\nControlRotation: P={cr.pitch:.1f} Y={cr.yaw:.1f} R={cr.roll:.1f}')
            print(f'PawnLoc: {pawn.get_actor_location()}')
