"""Revert F5 wrong changes + investigate F1 and the correct F5 walls.

1. Revert F5_SM_Cube82 / F5_SM_Cube143 to original positions
2. Delete F5_SM_StairwellReturn_A / F5_SM_StairwellReturn_B
3. Probe the EXPECTED F1 and F5 wall positions (same XY as F4, different Z)
4. Report what we find
"""
import unreal, json, os

PROJECT = r"C:/Users/Ommei/workspace/Signal"

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
by_label = {a.get_actor_label(): a for a in sma}

# 1) Revert F5 wrong moves
revert_targets = {
    "F5_SM_Cube82":  (994.4, -1473.2, 1740.9),
    "F5_SM_Cube143": (-3437.3, 5268.4, 1740.9),
}
print("=== Reverting wrong F5 moves ===")
for lbl, (x, y, z) in revert_targets.items():
    a = by_label.get(lbl)
    if a:
        cur = a.get_actor_location()
        a.set_actor_location(unreal.Vector(x, y, z), sweep=False, teleport=True)
        after = a.get_actor_location()
        print(f"  REVERT {lbl}: ({cur.x:.1f},{cur.y:.1f},{cur.z:.1f}) -> ({after.x:.1f},{after.y:.1f},{after.z:.1f})")
    else:
        print(f"  MISS: {lbl}")

# 2) Delete F5 wrong return pieces
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
print("\n=== Deleting wrong F5 return pieces ===")
for lbl in ("F5_SM_StairwellReturn_A", "F5_SM_StairwellReturn_B"):
    a = by_label.get(lbl)
    if a:
        ok = eas.destroy_actor(a)
        print(f"  DELETE {lbl}: ok={ok}")
    else:
        print(f"  MISS: {lbl} (already gone?)")

# 3) Probe expected F1 and F5 wall positions
# F4 reference: stairwell A wall at (355, -1473, 1701), stairwell B at (-2809, 5268, 1701)
# F5 expected: same XY, Z=1701+500=2201
# F1 expected: same XY, Z=1701-1500=201

print("\n=== Probing expected F1 + F5 wall positions ===")
probe_points = {
    "F1_A_expected": (355, -1473, 201),
    "F1_B_expected": (-2809, 5268, 201),
    "F5_A_expected": (355, -1473, 2201),
    "F5_B_expected": (-2809, 5268, 2201),
}

for name, (x, y, z) in probe_points.items():
    overlaps = unreal.SystemLibrary.box_overlap_actors(
        gw, unreal.Vector(x, y, z),
        unreal.Vector(50, 50, 50),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1,
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2],
        None, [])
    print(f"\n{name} probe @ ({x},{y},{z}) ±50:")
    cubes_at_loc = []
    for h in overlaps:
        lbl = h.get_actor_label()
        smc = h.get_component_by_class(unreal.StaticMeshComponent) if hasattr(h, 'get_component_by_class') else None
        mesh = smc.static_mesh.get_name() if (smc and smc.static_mesh) else "?"
        bo, be = h.get_actor_bounds(only_colliding_components=True)
        # Only print SM_Cube actors with wall-slab shape
        if mesh == "SM_Cube" and be.x < 20 and 100 < be.y < 200 and 200 < be.z < 350:
            cubes_at_loc.append((lbl, bo, be))
        elif "MapVolume" not in lbl:
            print(f"    other: {lbl}[{mesh}] @({bo.x:.0f},{bo.y:.0f},{bo.z:.0f}) ±({be.x:.0f},{be.y:.0f},{be.z:.0f})")
    for lbl, bo, be in cubes_at_loc:
        print(f"  WALL CANDIDATE: {lbl} @({bo.x:.0f},{bo.y:.0f},{bo.z:.0f}) ±({be.x:.0f},{be.y:.0f},{be.z:.0f})")

# Also probe for SM_Steps actors at all 4 floor transitions (F1->F2 ... F4->F5)
print("\n=== Probing for stair flights at each transition ===")
for transition_name, z_center in [("F1->F2", 451), ("F2->F3", 951), ("F3->F4", 1451), ("F4->F5", 1951)]:
    print(f"\n{transition_name} (Z~{z_center}):")
    for a in sma:
        smc = a.get_component_by_class(unreal.StaticMeshComponent) if hasattr(a, 'get_component_by_class') else None
        if not smc or not smc.static_mesh or smc.static_mesh.get_name() != "SM_Steps":
            continue
        bo, be = a.get_actor_bounds(only_colliding_components=True)
        if abs(bo.z - z_center) < 200:
            print(f"  {a.get_actor_label():22s} @({bo.x:.0f},{bo.y:.0f},{bo.z:.0f})")

print("\nDone.")
