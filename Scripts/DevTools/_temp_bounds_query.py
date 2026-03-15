import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
min_x = 999999.0
max_x = -999999.0
min_y = 999999.0
max_y = -999999.0
count = 0
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    loc = a.get_actor_location()
    if abs(loc.z - 2100.0) > 300:
        continue
    if loc.x < min_x: min_x = loc.x
    if loc.x > max_x: max_x = loc.x
    if loc.y < min_y: min_y = loc.y
    if loc.y > max_y: max_y = loc.y
    count += 1
min_x -= 200
min_y -= 200
max_x += 200
max_y += 200
print(f"BOUNDS:{min_x:.0f},{min_y:.0f},{max_x:.0f},{max_y:.0f},{count}")
