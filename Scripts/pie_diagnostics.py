import unreal

editor_subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor_subsys.get_game_world()
if not world:
    print("ERROR: No PIE world")
else:
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Character)
    for actor in actors:
        print(f"=== {actor.get_name()} ===")

        # Hidden Mesh
        mesh = actor.get_editor_property("Mesh")
        if mesh:
            sm = mesh.get_editor_property("SkeletalMeshAsset")
            print(f"HiddenMesh.Skel: {sm.get_name() if sm else 'NONE'}")
            print(f"HiddenMesh.AnimMode: {mesh.get_editor_property('AnimationMode')}")
            ai = mesh.get_anim_instance()
            print(f"HiddenMesh.AnimInst: {ai.get_class().get_name() if ai else 'NONE'}")
            print(f"HiddenMesh.IsPlaying: {ai.is_playing() if ai and hasattr(ai, 'is_playing') else 'N/A'}")

        # PlayerMesh
        try:
            pm = actor.get_editor_property("PlayerMesh")
            if pm:
                sm = pm.get_editor_property("SkeletalMeshAsset")
                print(f"PlayerMesh.Skel: {sm.get_name() if sm else 'NONE'}")
                pai = pm.get_anim_instance()
                print(f"PlayerMesh.AnimInst: {pai.get_class().get_name() if pai else 'NONE'}")
                if pai:
                    try:
                        src = pai.get_editor_property("SourceMeshComponent")
                        print(f"PlayerAnimInst.SourceMesh: {src.get_name() if src else 'NONE'}")
                        if src:
                            src_sm = src.get_editor_property("SkeletalMeshAsset")
                            print(f"SourceMesh.Skel: {src_sm.get_name() if src_sm else 'NONE'}")
                            src_ai = src.get_anim_instance()
                            print(f"SourceMesh.AnimInst: {src_ai.get_class().get_name() if src_ai else 'NONE'}")
                    except Exception as e:
                        print(f"SourceMesh error: {e}")

                    # Check linked anim layers
                    try:
                        linked = pm.get_linked_anim_graph_instances_by_tag("None")
                        print(f"LinkedAnimGraphs: {len(linked) if linked else 0}")
                    except:
                        pass
            else:
                print("PlayerMesh: NONE!")
        except Exception as e:
            print(f"PlayerMesh error: {e}")

        vel = actor.get_velocity()
        print(f"Speed2D: {(vel.x**2 + vel.y**2)**0.5:.1f}")
