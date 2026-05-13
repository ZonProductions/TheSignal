"""Revert the stairwell widen using the snapshot file.

Reverses:
  - Wall moves: set_actor_location back to before
  - Spawns: destroy the return-piece actors
  - Deletions: re-spawn the duplicate pillar(s) at their captured transform
"""
import unreal, json, os, glob

PROJECT = r"C:/Users/Ommei/workspace/Signal"
SNAP_DIR = os.path.join(PROJECT, ".zon-portal", "level_snapshots")
snap_files = sorted(glob.glob(os.path.join(SNAP_DIR, "*_all_stairwell_widen.json")))
if not snap_files:
    print("No snapshot found — nothing to revert"); raise SystemExit
snap_path = snap_files[-1]
print(f"Reverting from: {snap_path}")
with open(snap_path) as f:
    snap = json.load(f)

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
by_label = {a.get_actor_label(): a for a in sma}
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sm_cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")

# 1) Destroy spawned return pieces FIRST (avoid name collisions if we re-run)
print("\n=== Destroying spawned return pieces ===")
for s in snap.get("spawns", []):
    lbl = s["label"]
    a = by_label.get(lbl)
    if a:
        ok = eas.destroy_actor(a)
        print(f"  DELETE {lbl}: ok={ok}")
    else:
        print(f"  (already gone) {lbl}")

# Refresh actor list after deletions
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
by_label = {a.get_actor_label(): a for a in sma}

# 2) Move walls back to before
print("\n=== Moving walls back ===")
for m in snap.get("moves", []):
    lbl = m["label"]
    a = by_label.get(lbl)
    if not a:
        print(f"  MISS: {lbl} not found in world")
        continue
    bx, by_, bz = m["before_location"]
    cur = a.get_actor_location()
    a.set_actor_location(unreal.Vector(bx, by_, bz), sweep=False, teleport=True)
    after = a.get_actor_location()
    print(f"  REVERT {lbl}: ({cur.x:.1f},{cur.y:.1f},{cur.z:.1f}) -> ({after.x:.1f},{after.y:.1f},{after.z:.1f})")

# 3) Re-spawn deleted duplicate pillars
print("\n=== Re-spawning deleted duplicate pillars ===")
for d in snap.get("deletions", []):
    lbl = d["label"]
    loc = unreal.Vector(*d["location"])
    scale = unreal.Vector(*d["scale"])
    rot_p, rot_y, rot_r = d["rotation"]
    rot = unreal.Rotator(rot_p, rot_y, rot_r)
    new_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if not new_actor:
        print(f"  SPAWN FAIL for {lbl}")
        continue
    new_actor.set_actor_scale3d(scale)
    if sm_cube:
        new_actor.static_mesh_component.set_static_mesh(sm_cube)
    new_actor.set_actor_label(lbl)
    print(f"  RESTORE {lbl} @({loc.x:.1f},{loc.y:.1f},{loc.z:.1f}) scale=({scale.x:.2f},{scale.y:.2f},{scale.z:.2f})")

# Rename the snapshot to mark it as reverted (don't leave a "live snapshot" that suggests changes are active)
reverted_path = snap_path.replace(".json", "_REVERTED.json")
os.rename(snap_path, reverted_path)
print(f"\nSnapshot file moved to: {reverted_path}")

print("\n=== Revert complete ===")
print("DO NOT save the level yet — verify visually that everything is back to original first.")
print("If the level appears correct, you can Ctrl-S to persist the revert.")
print("If anything looks wrong, close the editor WITHOUT saving and reopen — the unsaved revert will be discarded.")
