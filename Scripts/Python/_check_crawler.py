import unreal

for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    name = actor.get_name()
    if "Patrol" in name or "Crawler" in name or "Monster_Pawn" in name:
        capsule = actor.get_component_by_class(unreal.CapsuleComponent)
        if capsule:
            print(f"{name}: collision={capsule.get_collision_enabled()}, object_type={capsule.get_collision_object_type()}, half_height={capsule.get_scaled_capsule_half_height():.0f}, radius={capsule.get_scaled_capsule_radius():.0f}")
        else:
            print(f"{name}: NO CAPSULE")

        # Check health comp
        health = actor.get_component_by_class(unreal.load_class(None, "/Script/TheSignal.ZP_HealthComponent"))
        if health:
            print(f"  HealthComp: MaxHealth={health.get_editor_property('MaxHealth')}, CurrentHealth={health.get_editor_property('CurrentHealth')}, bIsDead={health.get_editor_property('bIsDead')}")
        else:
            print(f"  NO HealthComp found")
