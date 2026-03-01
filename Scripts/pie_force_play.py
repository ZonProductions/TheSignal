import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)
for a in actors:
    mesh = a.get_editor_property("Mesh")
    ai = mesh.get_anim_instance()

    # Check if playing
    try:
        playing = ai.is_any_montage_playing()
        print(f"is_any_montage_playing: {playing}")
    except:
        pass

    # Try to read playing state
    for prop in ['b_playing', 'playing', 'b_looping', 'looping', 'play_rate']:
        try:
            v = getattr(ai, prop, None)
            if v is not None:
                print(f"  attr {prop}: {v}")
        except:
            pass

    # Force play
    print("Forcing: set_playing(True), set_looping(True), set_play_rate(1.0)")
    ai.set_playing(True)
    ai.set_looping(True)
    ai.set_play_rate(1.0)
    ai.set_blend_space_position(unreal.Vector(200, 0, 0))
    print("Done — check if legs are moving now")
