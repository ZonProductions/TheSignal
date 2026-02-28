import unreal

ess = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ess.get_game_world()
if not world:
    print("ERROR: No game world")
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    if not pawn:
        print("ERROR: No pawn")
    else:
        # Camera details
        cam_comps = pawn.get_components_by_class(unreal.CameraComponent)
        for cam in cam_comps:
            print(f'Camera "{cam.get_name()}":')
            print(f'  Parent: {cam.get_attach_parent().get_name() if cam.get_attach_parent() else "NONE"}')
            print(f'  Socket: {cam.get_attach_socket_name()}')
            print(f'  WorldLoc: {cam.get_world_location()}')
            print(f'  WorldRot: {cam.get_world_rotation()}')
            print(f'  AbsoluteRotation: {cam.get_editor_property("absolute_rotation")}')
            print(f'  RelLoc: {cam.get_editor_property("relative_location")}')
            print(f'  RelRot: {cam.get_editor_property("relative_rotation")}')
        
        # AC_FirstPersonCamera
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
        
        # Control rotation
        cr = pc.get_control_rotation()
        print(f'\nControlRotation: P={cr.pitch:.1f} Y={cr.yaw:.1f} R={cr.roll:.1f}')
        print(f'PawnLoc: {pawn.get_actor_location()}')
        print(f'PawnRot: {pawn.get_actor_rotation()}')
        
        # Compare camera world pos vs FPCamera socket world pos
        print(f'\n=== COMPARISON ===')
        for sk in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
            if sk.does_socket_exist('FPCamera'):
                tw = sk.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_WORLD)
                print(f'FPCamera Socket World: Loc={tw.translation}')
                print(f'FPCamera Socket World: Rot={tw.rotation.rotator()}')
        for cam in cam_comps:
            print(f'Camera World: Loc={cam.get_world_location()}')
            print(f'Camera World: Rot={cam.get_world_rotation()}')
        print(f'Controller:   Rot=P={cr.pitch:.1f} Y={cr.yaw:.1f} R={cr.roll:.1f}')
