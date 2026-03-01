import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)[0]
mesh = a.get_editor_property("Mesh")
ai = mesh.get_anim_instance()

walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F")
print(f"Walk anim: {walk.get_name() if walk else 'NONE'}")

if walk:
    # Use set_animation_asset instead of play_anim
    ai.set_animation_asset(walk)
    ai.set_looping(True)
    ai.set_play_rate(1.0)
    ai.set_playing(True)
    print("Forced walk anim on hidden mesh - CHECK IF LEGS MOVE")
