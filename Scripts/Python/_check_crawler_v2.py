import unreal

for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    name = actor.get_name()
    if "Patrol" in name or "Crawler" in name:
        capsule = actor.get_component_by_class(unreal.CapsuleComponent)
        if capsule:
            profile = capsule.get_collision_profile_name()
            enabled = capsule.get_collision_enabled()
            obj_type = capsule.get_collision_object_type()
            # Check specific channel responses
            resp_vis = capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY)
            resp_cam = capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA)
            resp_pawn = capsule.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)
            print(f"{name}: profile={profile}, enabled={enabled}, obj_type={obj_type}")
            print(f"  Visibility={resp_vis}, Camera={resp_cam}, Pawn={resp_pawn}")
            print(f"  half_height={capsule.get_scaled_capsule_half_height():.0f}, radius={capsule.get_scaled_capsule_radius():.0f}")
