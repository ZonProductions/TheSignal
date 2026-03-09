"""Find remaining floor/ceiling layers covering the stairwell.
Uses actor bounds to find large meshes that extend into the stairwell from anywhere."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Stairwell area: X 800-1050, Y -1500 to -1150
STAIR_X_MIN, STAIR_X_MAX = 800, 1050
STAIR_Y_MIN, STAIR_Y_MAX = -1500, -1150

# Floor-level Z heights for F2-F4 (and slightly above/below for ceiling/floor slabs)
FLOOR_ZS = [(400, 520, 2), (900, 1020, 3), (1400, 1520, 4)]

print("=== Remaining floor/ceiling actors overlapping stairwell ===\n")

for z_min, z_max, floor_num in FLOOR_ZS:
    print(f"--- Floor {floor_num} level (Z {z_min}-{z_max}) ---")
    found = 0

    for a in all_actors:
        cls = a.get_class().get_name()
        if cls != 'StaticMeshActor':
            continue

        # Get world-space bounds
        success, origin, extent = a.get_actor_bounds(False)
        if not success:
            continue

        # Actor Z range
        a_z_min = origin.z - extent.z
        a_z_max = origin.z + extent.z

        # Must be at this floor level
        if a_z_max < z_min or a_z_min > z_max:
            continue

        # Must overlap stairwell XY
        a_x_min = origin.x - extent.x
        a_x_max = origin.x + extent.x
        a_y_min = origin.y - extent.y
        a_y_max = origin.y + extent.y

        if a_x_max < STAIR_X_MIN or a_x_min > STAIR_X_MAX:
            continue
        if a_y_max < STAIR_Y_MIN or a_y_min > STAIR_Y_MAX:
            continue

        # This actor overlaps the stairwell at floor level
        label = a.get_actor_label()
        loc = a.get_actor_location()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mesh_name = ""
        if smc:
            sm = smc.get_editor_property("static_mesh")
            if sm:
                mesh_name = sm.get_name()
        print(f"  {label:50s} | {mesh_name:25s} | pos=({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) | extent=({extent.x:.0f},{extent.y:.0f},{extent.z:.0f})")
        found += 1

    if found == 0:
        print("  (none found)")
    print()
