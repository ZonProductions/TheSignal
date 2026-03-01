import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)[0]
mesh = a.get_editor_property("Mesh")

# Make hidden mesh visible
mesh.set_visibility(True)
mesh.set_only_owner_see(False)
print(f"Hidden mesh now VISIBLE")
print(f"SkeletalMesh: {mesh.get_editor_property('SkeletalMeshAsset').get_name()}")
print(f"AnimMode: {mesh.get_editor_property('AnimationMode')}")

ai = mesh.get_anim_instance()
print(f"AnimInst: {ai.get_class().get_name()}")

# Force walk anim
walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F")
if walk:
    ai.set_animation_asset(walk)
    ai.set_looping(True)
    ai.set_play_rate(1.0)
    ai.set_playing(True)
    print(f"Forced walk anim: {walk.get_name()}")
    print("LOOK DOWN — you should see the UEFN mannequin walking at your feet")
