import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)
for a in actors:
    mesh = a.get_editor_property("Mesh")
    ai = mesh.get_anim_instance()
    # Find play/loop related methods
    props = [p for p in dir(ai) if ('play' in p.lower() or 'blend' in p.lower() or 'loop' in p.lower() or 'position' in p.lower()) and not p.startswith('_')]
    print(f"Relevant methods: {props}")

    # Try to check playing state
    for name in ['b_playing', 'is_playing', 'b_looping', 'is_looping', 'current_position', 'play_rate']:
        try:
            v = ai.get_editor_property(name)
            print(f"  {name}: {v}")
        except:
            pass

    # Try calling methods
    try:
        playing = ai.is_playing()
        print(f"  is_playing(): {playing}")
    except:
        pass

    try:
        t = ai.get_current_time()
        print(f"  get_current_time(): {t}")
    except:
        pass
