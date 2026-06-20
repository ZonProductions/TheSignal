import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ues.get_game_world()  # PIE world
print("WORLD:", world.get_name() if world else None)

cls = unreal.load_object(None, "/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_Character.CC_Customizable_Character_C")
actors = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
print("CC actors in PIE:", len(actors))

for act in actors:
    print("==== ACTOR", act.get_actor_label(), act.get_name(), "====")
    comps = act.get_components_by_class(unreal.SkeletalMeshComponent)
    for c in comps:
        sm = c.get_skeletal_mesh_asset() if hasattr(c, "get_skeletal_mesh_asset") else None
        leader = c.get_editor_property("leader_pose_component") if False else None
        print("  COMP", c.get_name(), "mesh:", sm.get_path_name() if sm else None,
              "vis:", c.is_visible())
        # materials
        mats = c.get_materials()
        for i, m in enumerate(mats):
            print("      mat[%d]:" % i, m.get_name() if m else None)
    # also list ALL components to catch groom/apparel
    allc = act.get_components_by_class(unreal.ActorComponent)
    print("  ALL COMPONENT TYPES:", sorted(set(type(x).__name__ for x in allc)))
