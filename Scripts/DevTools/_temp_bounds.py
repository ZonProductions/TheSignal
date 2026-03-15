import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
texture = unreal.load_asset("/Game/Core/Maps/T_Map_F5")
found = 0
for a in eas.get_all_level_actors():
    if "MapPickup" not in a.get_class().get_name():
        continue
    z = a.get_actor_location().z
    if z < 1900 or z > 2300:
        continue
    a.set_editor_property("MapTexture", texture)
    a.set_editor_property("MapBoundsMin", unreal.Vector2D(-4960.9, -1950.0))
    a.set_editor_property("MapBoundsMax", unreal.Vector2D(2507.0, 5745.2))
    a.set_editor_property("AreaDisplayName", unreal.Text("Floor 5"))
    found += 1
    print("Set F5 MapPickup: " + a.get_name() + " bounds (-4961,-1950) to (2507,5745)")
if not found:
    print("WARNING: No MapPickup found on floor 5 (Z 1900-2300)")
