import unreal
from collections import defaultdict

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# The building uses SM_Cube (100x100x100 UU) as wall/floor tiles
# 1 UU = 1 cm, so 100 UU = 1 meter. Each SM_Cube tile = 1m x 1m
# Original building: Y < 1898
# Mirrored building: Y > 1898
# Patio/transition: area between the two OutsideFloor edges

# First, find the exact boundaries using SM_Cube actors (structural tiles)
# Group by floor (Z) and wing (Y relative to mirror line)

# Floor Z ranges
floors = {
    "F1": (-50, 200),
    "F2": (400, 700),
    "F3": (900, 1200),
    "F4": (1400, 1700),
    "F5": (1900, 2200),
}

# Find OutsideFloor Y positions to define patio boundaries
outside_floor_ys = set()
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if sm and sm.get_name() == "SM_OutsideFloor":
        outside_floor_ys.add(round(a.get_actor_location().y))

print(f"OutsideFloor Y positions: {sorted(outside_floor_ys)}")

# Collect all structural floor/wall tiles (SM_Cube) per floor
for floor_name, (z_min, z_max) in floors.items():
    left_wing = {"min_x": 99999, "max_x": -99999, "min_y": 99999, "max_y": -99999, "count": 0}
    patio = {"min_x": 99999, "max_x": -99999, "min_y": 99999, "max_y": -99999, "count": 0}
    right_wing = {"min_x": 99999, "max_x": -99999, "min_y": 99999, "max_y": -99999, "count": 0}

    for a in eas.get_all_level_actors():
        if a.get_class().get_name() != "StaticMeshActor":
            continue
        loc = a.get_actor_location()
        if loc.z < z_min or loc.z > z_max:
            continue
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm:
            continue
        mn = sm.get_name()
        # Only count structural floor/wall pieces
        if mn not in ("SM_Cube", "SM_Woodfloor", "SM_KitchenFloor", "SM_OutsideFloor"):
            continue

        # Categorize by wing
        # Original building interior: Y < ~1200 (before OutsideFloor)
        # Patio: Y ~1200 to ~2600 (between the two OutsideFloors)
        # Mirrored building interior: Y > ~2600
        if loc.y < 1200:
            section = left_wing
        elif loc.y > 2600:
            section = right_wing
        else:
            section = patio

        section["count"] += 1
        section["min_x"] = min(section["min_x"], loc.x)
        section["max_x"] = max(section["max_x"], loc.x)
        section["min_y"] = min(section["min_y"], loc.y)
        section["max_y"] = max(section["max_y"], loc.y)

    print(f"\n=== {floor_name} ===")
    for name, s in [("LEFT WING (original)", left_wing), ("PATIO", patio), ("RIGHT WING (mirror)", right_wing)]:
        if s["count"] == 0:
            print(f"  {name}: empty")
            continue
        width_uu = s["max_x"] - s["min_x"]
        depth_uu = s["max_y"] - s["min_y"]
        # 100 UU = 1 meter = ~3.28 ft. 1 tile = 1m x 1m
        width_m = width_uu / 100.0
        depth_m = depth_uu / 100.0
        width_tiles = round(width_uu / 100)
        depth_tiles = round(depth_uu / 100)
        sqft = width_m * depth_m * 10.764  # sq meters to sq feet
        print(f"  {name}:")
        print(f"    Tiles: {s['count']} structural")
        print(f"    Extent: X {s['min_x']:.0f} to {s['max_x']:.0f}, Y {s['min_y']:.0f} to {s['max_y']:.0f}")
        print(f"    Size: {width_tiles} x {depth_tiles} tiles ({width_m:.0f}m x {depth_m:.0f}m)")
        print(f"    Area: ~{sqft:.0f} sqft ({width_m*depth_m:.0f} sqm)")
