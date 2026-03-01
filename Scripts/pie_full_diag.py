import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)
for a in actors:
    vel = a.get_velocity()
    speed = (vel.x**2 + vel.y**2)**0.5
    print(f"Speed2D: {speed:.1f}")

    mesh = a.get_editor_property("Mesh")
    ai = mesh.get_anim_instance()
    print(f"HiddenMesh AnimInst: {ai.get_class().get_name()}")

    # Force play and check
    ai.set_playing(True)
    ai.set_looping(True)
    ai.set_blend_space_position(unreal.Vector(200, 0, 0))

    # Sample bone positions twice with a frame gap
    foot1 = mesh.get_socket_location("foot_l")
    print(f"foot_l BEFORE: ({foot1.x:.2f}, {foot1.y:.2f}, {foot1.z:.2f})")

    # Check what animation asset is actually set
    # Use play_anim to see if it works differently
    lbs = unreal.load_asset("/Game/Core/Player/BS_Grace_Locomotion")
    print(f"BS_Grace_Locomotion loaded: {lbs is not None}")
    if lbs:
        print(f"BS class: {lbs.get_class().get_name()}")
        # Try setting it again at runtime
        ai.set_animation_asset(lbs, True, 1.0)
        ai.set_playing(True)
        ai.set_blend_space_position(unreal.Vector(200, 0, 0))
        print("Re-set animation asset + playing + position")

    # Now check PlayerMesh
    pm = a.get_editor_property("PlayerMesh")
    pai = pm.get_anim_instance()
    print(f"PlayerMesh AnimInst: {pai.get_class().get_name()}")
    src = pai.get_editor_property("SourceMeshComponent")
    print(f"SourceMesh: {src.get_name() if src else 'NONE'}")

    # Check linked anim graph on player mesh
    try:
        linked = pm.get_linked_anim_graph_instance_by_tag("None")
        print(f"LinkedAnimGraph: {linked}")
    except:
        pass

    # Check PlayerMesh foot
    pfoot = pm.get_socket_location("foot_l")
    print(f"PlayerMesh foot_l: ({pfoot.x:.2f}, {pfoot.y:.2f}, {pfoot.z:.2f})")
