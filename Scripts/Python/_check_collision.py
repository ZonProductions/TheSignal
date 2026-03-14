import unreal

for a in unreal.EditorLevelLibrary.get_all_level_actors():
    n = a.get_class().get_name()
    if "CardReader" in n or "KeyPickup" in n:
        loc = a.get_actor_location()
        print(f"{a.get_name()} [{n}] at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            ct = c.get_collision_enabled()
            cp = c.get_collision_profile_name()
            print(f"  comp: {c.get_name()} [{type(c).__name__}] collision={ct} profile={cp}")
