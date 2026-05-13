"""Widen the L-turn landing on BOTH stairwells, ALL 5 floors.

Plan per floor per stairwell:
  1. Identify the outer wall slab (Cube60-style — extent ~9x146x275 SM_Cube).
  2. Identify its corner pillar (Cube59-style — extent ~9x9x275 SM_Cube), and
     any duplicate stacked on top of it (Cube122-style).
  3. Snapshot before-state.
  4. Move the wall 30 UU OUTWARD (away from its stair, along world X).
  5. Spawn a perpendicular "return" wall bridging the new wall to the pillar.
  6. Delete the duplicate pillar.

Does NOT save the level — dev reviews visually then Ctrl-S.
"""
import unreal, json, os
from datetime import datetime

PROJECT = r"C:/Users/Ommei/workspace/Signal"
SNAP_DIR = os.path.join(PROJECT, ".zon-portal", "level_snapshots")
os.makedirs(SNAP_DIR, exist_ok=True)
ts = datetime.now().strftime("%Y-%m-%dT%H-%M-%S")
snap_path = os.path.join(SNAP_DIR, f"{ts}_all_stairwell_widen.json")

PUSH = 30.0  # UU outward

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
print(f"World: {gw}")

sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)

# Match wall slab vs pillar vs ordinary cube by extent shape.
def is_wall_slab(ext):  # 18 × 292 × 550 dims → extent ~9 × 146 × 275
    return ext.x < 20 and 100 < ext.y < 200 and 200 < ext.z < 350

def is_pillar(ext):     # 18 × 18 × 550 dims → extent ~9 × 9 × 275
    return ext.x < 20 and ext.y < 20 and 200 < ext.z < 350

# Group F*_SM_Cube* actors by floor prefix
walls_by_floor   = {f"F{n}": [] for n in range(1, 6)}
pillars_by_floor = {f"F{n}": [] for n in range(1, 6)}
for a in sma:
    lbl = a.get_actor_label()
    if not lbl.startswith("F") or "Cube" not in lbl:
        continue
    floor = lbl.split("_", 1)[0]
    if floor not in walls_by_floor:
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc or not smc.static_mesh or smc.static_mesh.get_name() != "SM_Cube":
        continue
    origin, extent = a.get_actor_bounds(only_colliding_components=True)
    if is_wall_slab(extent):
        walls_by_floor[floor].append((lbl, a, origin, extent))
    elif is_pillar(extent):
        pillars_by_floor[floor].append((lbl, a, origin, extent))

# Reference positions from F4 (known good)
# Stairwell A (steps2 side): wall around world X=+355, Y=-1473  -> push -X
# Stairwell B (steps3 side): wall around world X=-2809, Y=+5268 -> push +X
def stairwell_for(origin):
    if origin.x > 0:  return "A", -1.0  # stairwell A, push -X
    else:             return "B", +1.0  # stairwell B, push +X

# Build action plan
plan = []  # entries: dict(wall=label, push_dir=±1, pillar=label_or_None, dup=label_or_None, return_loc=Vector, return_scale=Vector)
log = []

snap = {
    "timestamp": ts,
    "purpose": "Widen L-turn landing on both stairwells, F1-F5. Push outer walls 30 UU outward, add corner return wall, delete duplicate pillars.",
    "push_uu": PUSH,
    "moves": [], "spawns": [], "deletions": [],
}

print(f"\n=== Discovered walls and pillars ===")
for floor in sorted(walls_by_floor.keys()):
    walls = walls_by_floor[floor]
    pillars = pillars_by_floor[floor]
    print(f"\n{floor}: walls={len(walls)} pillars={len(pillars)}")
    # Identify Stairwell A and B wall on this floor
    stairwells_seen = {}
    for lbl, a, origin, extent in walls:
        sw, push_sign = stairwell_for(origin)
        stairwells_seen.setdefault(sw, []).append((lbl, a, origin, extent, push_sign))

    for sw, items in stairwells_seen.items():
        if len(items) > 1:
            print(f"  WARN: {floor} stairwell {sw} has {len(items)} candidate walls. Picking by Y proximity to F4 reference.")
            # F4 reference Y: stairwell A at Y=-1473, B at Y=+5268
            target_y = -1473.0 if sw == "A" else +5268.0
            items.sort(key=lambda t: abs(t[2].y - target_y))
        lbl_wall, a_wall, owall, ewall, push_sign = items[0]

        # Find the pillar that abuts this wall's +Y end (or appropriate end).
        # Wall Y range: [origin.y - extent.y, origin.y + extent.y]
        # The corner is the end with neighboring pillar(s) — find pillar closest to wall.
        nearest_pillar = None
        if pillars:
            def dist_to_wall_end(p_origin):
                # closest end of wall to pillar
                d_minus = ((p_origin.x - owall.x)**2 + (p_origin.y - (owall.y - ewall.y))**2)**0.5
                d_plus  = ((p_origin.x - owall.x)**2 + (p_origin.y - (owall.y + ewall.y))**2)**0.5
                return min(d_minus, d_plus), ("minus" if d_minus < d_plus else "plus")
            pillars_with_dist = [(p_lbl, p_a, p_origin, p_extent, dist_to_wall_end(p_origin)) for (p_lbl, p_a, p_origin, p_extent) in pillars]
            pillars_with_dist.sort(key=lambda t: t[4][0])
            nearest_pillar = pillars_with_dist[0]

        # Look for a duplicate pillar at the SAME location as nearest_pillar
        dup_label = None
        if nearest_pillar:
            np_lbl, np_a, np_origin, np_extent, (np_dist, np_end) = nearest_pillar
            for p_lbl, p_a, p_origin, p_extent in pillars:
                if p_lbl == np_lbl: continue
                if (abs(p_origin.x - np_origin.x) < 2 and
                    abs(p_origin.y - np_origin.y) < 2 and
                    abs(p_origin.z - np_origin.z) < 2):
                    dup_label = p_lbl
                    break

        print(f"  Stairwell {sw}: wall={lbl_wall} @({owall.x:.0f},{owall.y:.0f},{owall.z:.0f}) push_sign={push_sign:+.0f}")
        if nearest_pillar:
            print(f"    pillar={np_lbl} @({np_origin.x:.0f},{np_origin.y:.0f},{np_origin.z:.0f}) (corner at wall.{np_end}-Y end)")
        if dup_label:
            print(f"    duplicate pillar: {dup_label}  -> WILL DELETE")
        else:
            print(f"    (no duplicate pillar)")

        # Compute new wall location: move along world X by push_sign * PUSH
        wall_loc = a_wall.get_actor_location()
        new_wall_loc = unreal.Vector(wall_loc.x + push_sign * PUSH, wall_loc.y, wall_loc.z)

        # Compute corner-return piece transform:
        # The return is a perpendicular wall section bridging the gap between
        # the moved wall's outer face and the corner pillar.
        # It runs along world X, located at the corner-side Y end of the wall.
        # Length in X = PUSH + small overlap into pillar (~PUSH UU).
        # Position: at the +Y or -Y end of wall depending on np_end.
        if nearest_pillar:
            if np_end == "plus":
                corner_y = owall.y + ewall.y  # +Y end of wall
            else:
                corner_y = owall.y - ewall.y  # -Y end of wall
        else:
            # No pillar found; default to the +Y end (most common)
            corner_y = owall.y + ewall.y

        # The return spans X from new_wall_loc.x (its center after move) to wall_loc.x (the OLD center)
        # ie. the return covers the X distance the wall was moved.
        # Center X is midpoint of new and old wall center X.
        return_cx = (new_wall_loc.x + wall_loc.x) / 2.0
        return_cy = corner_y
        return_cz = wall_loc.z
        # Dimensions: X length = PUSH (to bridge), Y thickness = 18 UU (same as wall),
        # Z height = same as wall.
        # SM_Cube is 100 cube; for X=PUSH UU, scale_x = PUSH/100 = 0.30; for Y=18 UU, scale_y=0.18;
        # for Z=550 UU, scale_z=5.50
        return_scale = unreal.Vector(PUSH / 100.0, 0.18, 5.50)
        return_loc = unreal.Vector(return_cx, return_cy, return_cz)

        plan.append({
            "floor": floor, "stairwell": sw,
            "wall_actor": a_wall, "wall_label": lbl_wall,
            "before_wall_loc": (wall_loc.x, wall_loc.y, wall_loc.z),
            "after_wall_loc":  (new_wall_loc.x, new_wall_loc.y, new_wall_loc.z),
            "return_loc": (return_loc.x, return_loc.y, return_loc.z),
            "return_scale": (return_scale.x, return_scale.y, return_scale.z),
            "dup_actor": next((a for (l, a, _o, _e) in pillars if l == dup_label), None),
            "dup_label": dup_label,
        })

print(f"\n=== Plan summary ===")
print(f"  {len(plan)} (floor, stairwell) edits to apply.")
print(f"  Each: move wall {PUSH} UU outward + spawn return piece + delete dup pillar (if any).")

# === Now APPLY ===
print(f"\n=== Applying changes ===")

# 1. Move walls
for p in plan:
    a = p["wall_actor"]
    bx, by, bz = p["before_wall_loc"]
    ax, ay, az = p["after_wall_loc"]
    success = a.set_actor_location(unreal.Vector(ax, ay, az), sweep=False, teleport=True)
    print(f"  MOVE {p['wall_label']:20s}  ({bx:7.1f},{by:7.1f},{bz:6.1f}) -> ({ax:7.1f},{ay:7.1f},{az:6.1f})  ok={success}")
    snap["moves"].append({
        "label": p["wall_label"],
        "before_location": [bx, by, bz],
        "after_location":  [ax, ay, az],
    })

# 2. Spawn return pieces
sm_cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
if not sm_cube:
    print("  WARN: could not load /Engine/BasicShapes/Cube — return pieces will lack mesh")
spawned_actors = []
for p in plan:
    rl = unreal.Vector(*p["return_loc"])
    rs = unreal.Vector(*p["return_scale"])
    # Spawn StaticMeshActor at the return location
    new_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, rl, unreal.Rotator(0, 0, 0))
    if not new_actor:
        print(f"  SPAWN FAIL for {p['floor']} {p['stairwell']}")
        continue
    new_actor.set_actor_scale3d(rs)
    if sm_cube:
        new_actor.static_mesh_component.set_static_mesh(sm_cube)
    new_label = f"{p['floor']}_SM_StairwellReturn_{p['stairwell']}"
    new_actor.set_actor_label(new_label)
    print(f"  SPAWN {new_label} @({rl.x:.1f},{rl.y:.1f},{rl.z:.1f}) scale=({rs.x:.2f},{rs.y:.2f},{rs.z:.2f})")
    spawned_actors.append(new_label)
    snap["spawns"].append({
        "label": new_label,
        "location": [rl.x, rl.y, rl.z],
        "scale":    [rs.x, rs.y, rs.z],
        "mesh":     "/Engine/BasicShapes/Cube",
    })

# 3. Delete duplicate pillars
for p in plan:
    dup_a = p["dup_actor"]
    dup_label = p["dup_label"]
    if not dup_a:
        continue
    # Capture state before delete
    dloc = dup_a.get_actor_location()
    dscale = dup_a.get_actor_scale3d()
    drot = dup_a.get_actor_rotation()
    snap["deletions"].append({
        "label": dup_label,
        "location": [dloc.x, dloc.y, dloc.z],
        "scale":    [dscale.x, dscale.y, dscale.z],
        "rotation": [drot.pitch, drot.yaw, drot.roll],
        "mesh":     "SM_Cube",
    })
    success = unreal.EditorLevelLibrary.destroy_actor(dup_a)
    print(f"  DELETE {dup_label} (duplicate) ok={success}")

with open(snap_path, "w") as f:
    json.dump(snap, f, indent=2)
print(f"\nSnapshot saved: {snap_path}")
print(f"\nDone. Review in viewport (PIE off), then Ctrl-S to persist the level edits.")
