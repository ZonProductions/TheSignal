import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)
a = actors[0]

# Check if LocomotionBlendSpace is set at runtime
try:
    lbs = a.get_editor_property("LocomotionBlendSpace")
    print(f"LocomotionBlendSpace: {lbs.get_name() if lbs else 'NONE'}")
except Exception as e:
    print(f"LocomotionBlendSpace error: {e}")

# Check hidden mesh state
mesh = a.get_editor_property("Mesh")
ai = mesh.get_anim_instance()
print(f"AnimInst class: {ai.get_class().get_name()}")
print(f"AnimMode: {mesh.get_editor_property('AnimationMode')}")

# Check tick state
print(f"ComponentTickEnabled: {mesh.is_component_tick_enabled()}")
print(f"VisBasedAnimTick: {mesh.get_editor_property('VisibilityBasedAnimTickOption')}")
print(f"IsVisible: {mesh.is_visible()}")
print(f"WasRecentlyRendered: {mesh.was_recently_rendered(2.0)}")

# Try to directly play an anim sequence to test if the mesh can animate at all
idle = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Locomotion/M_Neutral_Stand_Idle_Loop")
if idle:
    print(f"Test idle anim: {idle.get_name()}")
    ai.play_anim(idle, True, 1.0)
    ai.set_playing(True)
    print("Set idle anim directly via play_anim - CHECK IF IDLE ANIMATES")
else:
    # Try finding an anim
    walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Locomotion/M_Neutral_Walk_Loop_F")
    if walk:
        print(f"Test walk anim: {walk.get_name()}")
        ai.play_anim(walk, True, 1.0)
        ai.set_playing(True)
        print("Set walk anim directly - CHECK IF WALK ANIMATES")
    else:
        print("Could not find test anims")
        # List what's in the locomotion folder
        reg = unreal.AssetRegistryHelpers.get_asset_registry()
        results = reg.get_assets_by_path("/Game/gasp_Characters/UEFN_Mannequin/Animations", recursive=True)
        for r in results[:10]:
            print(f"  Found: {r.package_name}")
