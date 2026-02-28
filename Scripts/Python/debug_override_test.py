import unreal, time

ess = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ess.get_game_world()
if not world:
    print("ERROR: No game world - start PIE first")
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if not pawn:
        print("ERROR: No pawn")
    else:
        cams = pawn.get_components_by_class(unreal.CameraComponent)
        for cam in cams:
            if cam.get_name() == 'FirstPersonCamera':
                # Read current
                rl = cam.get_editor_property('relative_location')
                rr = cam.get_editor_property('relative_rotation')
                print(f'BEFORE: RelLoc={rl}, RelRot={rr}')
                
                # Force to zero
                cam.set_editor_property('relative_location', unreal.Vector(0,0,0))
                cam.set_editor_property('relative_rotation', unreal.Rotator(0,0,0))
                
                rl2 = cam.get_editor_property('relative_location')
                print(f'AFTER SET: RelLoc={rl2}')
                
                # The tick hasn't run yet. Print a message asking user to re-run this
                print('Now re-run this script to see if it snapped back (AC_FPC tick would have run)')
