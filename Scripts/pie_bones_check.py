import unreal

editor_subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor_subsys.get_game_world()
if not world:
    print("ERROR: No PIE world")
else:
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Character)
    for actor in actors:
        vel = actor.get_velocity()
        speed = (vel.x**2 + vel.y**2)**0.5
        print(f"Speed2D: {speed:.1f}")

        mesh = actor.get_editor_property("Mesh")
        if mesh:
            for bone in ["pelvis", "thigh_l", "foot_l"]:
                try:
                    loc = mesh.get_socket_location(bone)
                    print(f"  Hidden.{bone}: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
                except Exception as e:
                    print(f"  Hidden.{bone}: {e}")

        try:
            pm = actor.get_editor_property("PlayerMesh")
            if pm:
                for bone in ["pelvis", "thigh_l", "foot_l"]:
                    try:
                        loc = pm.get_socket_location(bone)
                        print(f"  Player.{bone}: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
                    except Exception as e:
                        print(f"  Player.{bone}: {e}")
        except Exception as e:
            print(f"PlayerMesh: {e}")
