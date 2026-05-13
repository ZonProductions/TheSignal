"""Apply landing widen to F1 and F5 — the two floors the first pass missed.

F1: walls have NO floor prefix (SM_Cube12, SM_Cube54)
F5: walls are SHORTER (Z extent 231 instead of 275) — return piece height adjusted.

Also: check F1, F2, F3, F5 for duplicate pillars and remove them.
Appends to the same snapshot file from the first run.
"""
import unreal, json, os, glob

PROJECT = r"C:/Users/Ommei/workspace/Signal"
SNAP_DIR = os.path.join(PROJECT, ".zon-portal", "level_snapshots")
PUSH = 30.0

# Find the latest snapshot file from the first run to APPEND results
snap_files = sorted(glob.glob(os.path.join(SNAP_DIR, "*_all_stairwell_widen.json")))
snap_path = snap_files[-1] if snap_files else None
if snap_path and os.path.exists(snap_path):
    with open(snap_path) as f:
        snap = json.load(f)
    snap.setdefault("moves", [])
    snap.setdefault("spawns", [])
    snap.setdefault("deletions", [])
    print(f"Appending to snapshot: {snap_path}")
else:
    snap = {"moves": [], "spawns": [], "deletions": []}
    print("(no prior snapshot)")

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
by_label = {a.get_actor_label(): a for a in sma}

# Explicit targets per floor: (wall_label, stairwell, expected_corner_pillar_label, push_direction)
targets = [
    # F1 — unprefixed naming
    ("SM_Cube12",     "F1", "A", "SM_Cube11",   -1),  # F1 stairwell A: wall + pillar (guessing pillar name; verify in script)
    ("SM_Cube54",     "F1", "B", "SM_Cube53",   +1),
    # F5 — prefixed
    ("F5_SM_Cube60",  "F5", "A", "F5_SM_Cube59",  -1),
    ("F5_SM_Cube106", "F5", "B", "F5_SM_Cube105", +1),
]

# Find pillar labels by proximity to wall corner end instead of guessing
def find_corner_pillar(wall_actor, sw):
    """Find a pillar (extent ~9x9x*) near wall's corner Y-end."""
    wo, we = wall_actor.get_actor_bounds(only_colliding_components=True)
    best = None
    best_dist = 1e9
    # Corner end Y: for sw A, +Y end; for sw B, -Y end (per earlier discoveries)
    corner_y = wo.y + we.y if sw == "A" else wo.y - we.y
    for a in sma:
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if not smc or not smc.static_mesh or smc.static_mesh.get_name() != "SM_Cube":
            continue
        bo, be = a.get_actor_bounds(only_colliding_components=True)
        if be.x > 20 or be.y > 20 or be.z < 200:  # not a pillar shape
            continue
        # Z must be close (same floor)
        if abs(bo.z - wo.z) > 50:
            continue
        d = ((bo.x - wo.x)**2 + (bo.y - corner_y)**2)**0.5
        if d < best_dist:
            best_dist = d
            best = (a, bo, be, d)
    return best

print("\n=== Applying F1 + F5 changes ===")
for wall_label, floor, sw, _hint_pillar_label, push_sign in targets:
    a_wall = by_label.get(wall_label)
    if not a_wall:
        print(f"  MISS: {wall_label} not found in world")
        continue
    wall_loc = a_wall.get_actor_location()
    wall_scale = a_wall.get_actor_scale3d()
    wall_rot = a_wall.get_actor_rotation()
    wo, we = a_wall.get_actor_bounds(only_colliding_components=True)

    # Find pillar
    pillar = find_corner_pillar(a_wall, sw)
    pillar_info = ""
    if pillar:
        p_actor, p_origin, p_extent, p_dist = pillar
        pillar_info = f"pillar={p_actor.get_actor_label()} @({p_origin.x:.0f},{p_origin.y:.0f},{p_origin.z:.0f}) dist={p_dist:.0f}"
        # Find duplicate at same XYZ as pillar
        for a in sma:
            if a is p_actor: continue
            lbl = a.get_actor_label()
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            if not smc or not smc.static_mesh or smc.static_mesh.get_name() != "SM_Cube":
                continue
            bo, be = a.get_actor_bounds(only_colliding_components=True)
            if be.x > 20 or be.y > 20 or be.z < 200:
                continue
            if (abs(bo.x - p_origin.x) < 2 and abs(bo.y - p_origin.y) < 2 and abs(bo.z - p_origin.z) < 2):
                # Delete duplicate
                dloc = a.get_actor_location()
                dscale = a.get_actor_scale3d()
                drot = a.get_actor_rotation()
                snap["deletions"].append({
                    "label": lbl,
                    "location": [dloc.x, dloc.y, dloc.z],
                    "scale":    [dscale.x, dscale.y, dscale.z],
                    "rotation": [drot.pitch, drot.yaw, drot.roll],
                    "mesh":     "SM_Cube",
                })
                eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
                ok = eas.destroy_actor(a)
                print(f"  DELETE duplicate pillar {lbl}: ok={ok}")
    else:
        pillar_info = "(no pillar found)"

    # Move the wall
    new_x = wall_loc.x + push_sign * PUSH
    new_loc = unreal.Vector(new_x, wall_loc.y, wall_loc.z)
    success = a_wall.set_actor_location(new_loc, sweep=False, teleport=True)
    print(f"  MOVE {wall_label:18s} ({wall_loc.x:7.1f},{wall_loc.y:7.1f},{wall_loc.z:6.1f}) -> ({new_x:7.1f},{wall_loc.y:7.1f},{wall_loc.z:6.1f})  {pillar_info}")
    snap["moves"].append({
        "label": wall_label,
        "before_location": [wall_loc.x, wall_loc.y, wall_loc.z],
        "after_location":  [new_x, wall_loc.y, wall_loc.z],
    })

    # Spawn the corner-return piece — match the wall's height (Z extent might differ for F5)
    # we is wall extent BEFORE move; same dims now
    z_scale_for_return = (we.z * 2) / 100.0  # mesh is 100 UU base; we want full Z dim = 2*extent
    corner_y = wo.y + we.y if sw == "A" else wo.y - we.y
    return_cx = (new_x + wall_loc.x) / 2.0
    return_loc = unreal.Vector(return_cx, corner_y, wall_loc.z)
    return_scale = unreal.Vector(PUSH / 100.0, 0.18, z_scale_for_return)

    sm_cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    new_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, return_loc, unreal.Rotator(0, 0, 0))
    if new_actor:
        new_actor.set_actor_scale3d(return_scale)
        if sm_cube:
            new_actor.static_mesh_component.set_static_mesh(sm_cube)
        new_label = f"{floor}_SM_StairwellReturn_{sw}"
        new_actor.set_actor_label(new_label)
        print(f"  SPAWN {new_label} @({return_loc.x:.1f},{return_loc.y:.1f},{return_loc.z:.1f}) scale=({return_scale.x:.2f},{return_scale.y:.2f},{return_scale.z:.2f})")
        snap["spawns"].append({
            "label": new_label,
            "location": [return_loc.x, return_loc.y, return_loc.z],
            "scale":    [return_scale.x, return_scale.y, return_scale.z],
            "mesh":     "/Engine/BasicShapes/Cube",
        })

# Also check F2/F3 for duplicate pillars (we already did F4; F5 just covered above)
print("\n=== Hunting for any remaining duplicate pillars (F2/F3) ===")
pillar_positions = {}
for a in sma:
    lbl = a.get_actor_label()
    if not lbl.startswith(("F2_", "F3_")) or "Cube" not in lbl:
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc or not smc.static_mesh or smc.static_mesh.get_name() != "SM_Cube":
        continue
    bo, be = a.get_actor_bounds(only_colliding_components=True)
    if be.x > 20 or be.y > 20 or be.z < 200:
        continue
    key = (round(bo.x), round(bo.y), round(bo.z))
    pillar_positions.setdefault(key, []).append((lbl, a))

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
dup_count = 0
for key, items in pillar_positions.items():
    if len(items) > 1:
        # Keep the lowest-numbered one (typically the "original"), delete the rest
        items.sort(key=lambda t: t[0])
        keep = items[0][0]
        for lbl, a in items[1:]:
            dloc = a.get_actor_location()
            dscale = a.get_actor_scale3d()
            drot = a.get_actor_rotation()
            snap["deletions"].append({
                "label": lbl,
                "location": [dloc.x, dloc.y, dloc.z],
                "scale":    [dscale.x, dscale.y, dscale.z],
                "rotation": [drot.pitch, drot.yaw, drot.roll],
                "mesh":     "SM_Cube",
            })
            ok = eas.destroy_actor(a)
            print(f"  DELETE duplicate {lbl} (keep {keep}): ok={ok}")
            dup_count += 1
print(f"  Found and removed {dup_count} duplicates in F2/F3.")

if snap_path:
    with open(snap_path, "w") as f:
        json.dump(snap, f, indent=2)
    print(f"\nSnapshot updated: {snap_path}")
print("\nDone.")
