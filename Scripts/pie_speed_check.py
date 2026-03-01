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

        # Check hidden mesh anim instance state
        mesh = actor.get_editor_property("Mesh")
        if mesh:
            ai = mesh.get_anim_instance()
            if ai:
                # Check if it's playing and what position
                print(f"HiddenMesh.AnimInst class: {ai.get_class().get_name()}")
                # Try to get blend space position
                try:
                    pos = ai.get_editor_property("blend_space_position")
                    print(f"BlendSpacePosition: {pos}")
                except:
                    pass
                try:
                    pos = ai.get_editor_property("current_time")
                    print(f"CurrentTime: {pos}")
                except:
                    pass
                # Check if playing
                try:
                    playing = ai.get_editor_property("playing")
                    print(f"IsPlaying: {playing}")
                except:
                    pass
                # List readable props
                for prop in ['b_playing', 'playing', 'current_time', 'play_rate', 'current_asset']:
                    try:
                        v = ai.get_editor_property(prop)
                        print(f"  {prop}: {v}")
                    except:
                        pass

        # Check PlayerMesh retarget state
        try:
            pm = actor.get_editor_property("PlayerMesh")
            if pm:
                pai = pm.get_anim_instance()
                if pai:
                    src = pai.get_editor_property("SourceMeshComponent")
                    if src:
                        src_ai = src.get_anim_instance()
                        print(f"SourceMesh ticking: {src.is_registered()}")
                        # Check bone transforms to see if pose is changing
                        pelvis_loc = src.get_bone_location_by_name("pelvis", unreal.BoneSpaceName.WORLD_SPACE)
                        print(f"SourceMesh pelvis world: {pelvis_loc}")
                        # Check a leg bone
                        thigh_loc = src.get_bone_location_by_name("thigh_l", unreal.BoneSpaceName.WORLD_SPACE)
                        print(f"SourceMesh thigh_l world: {thigh_loc}")
        except Exception as e:
            print(f"PlayerMesh check error: {e}")
