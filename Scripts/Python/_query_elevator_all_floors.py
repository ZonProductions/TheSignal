"""Compare elevator area across all floors to find glass alignment issue."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Elevator wall position: X=1275.6, Y=-1316.1
# Search wider area for glass-related actors near elevator on each floor
ELEV_X = 1275.6
ELEV_Y = -1316.1
SEARCH_RADIUS_X = 500
SEARCH_RADIUS_Y = 100  # Narrow Y to find things at same depth as wall

for floor in range(1, 6):
    floor_z_base = (floor - 1) * 500
    z_min = floor_z_base - 50
    z_max = floor_z_base + 450

    print(f"\n=== FLOOR {floor} (Z {z_min} to {z_max}) — near elevator ===")
    nearby = []
    for a in all_actors:
        loc = a.get_actor_location()
        if abs(loc.x - ELEV_X) > SEARCH_RADIUS_X:
            continue
        if abs(loc.y - ELEV_Y) > SEARCH_RADIUS_Y:
            continue
        if loc.z < z_min or loc.z > z_max:
            continue

        cls = a.get_class().get_name()
        label = a.get_actor_label()
        mesh_name = ""
        mat_info = ""

        if cls == 'StaticMeshActor':
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            if smc:
                sm = smc.get_editor_property("static_mesh")
                if sm:
                    mesh_name = sm.get_name()
                mats = []
                for i in range(smc.get_num_materials()):
                    mat = smc.get_material(i)
                    if mat:
                        mats.append(mat.get_name())
                mat_info = ", ".join(mats)

        nearby.append((label, cls, loc, mesh_name, mat_info))

    nearby.sort(key=lambda x: x[2].x)
    for label, cls, loc, mesh, mats in nearby:
        print(f"  {label:50s} | {mesh:25s} | pos=({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f}) | {mats}")

    if not nearby:
        print("  (none)")
