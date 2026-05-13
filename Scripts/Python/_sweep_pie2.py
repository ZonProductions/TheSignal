"""Capsule overlap at the player's current PIE position — fixed PIE world access."""
import unreal, math

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world()
if not gw:
    print("No PIE game world — confirm PIE is running")
    raise SystemExit

pawns = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Pawn)
player = None
for p in pawns:
    if "Grace" in p.get_class().get_name() or "Grace" in p.get_actor_label():
        player = p
        break
if not player:
    print("No Grace pawn in PIE world")
    raise SystemExit

ploc = player.get_actor_location()
prot = player.get_actor_rotation()
cap = player.get_component_by_class(unreal.CapsuleComponent)
R = cap.get_scaled_capsule_radius() if cap else 34.0
H = cap.get_scaled_capsule_half_height() if cap else 88.0
print(f"GRACE @ ({ploc.x:.1f},{ploc.y:.1f},{ploc.z:.1f}) yaw={prot.yaw:.0f}  r={R:.1f} hh={H:.1f}")

# Also locate F4_SM_Steps2 in the PIE world to confirm geometry & compute local pos
steps = None
sma_actors = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
for a in sma_actors:
    if "F4_SM_Steps2" in a.get_actor_label():
        steps = a
        break
if steps:
    sloc = steps.get_actor_location()
    srot = steps.get_actor_rotation()
    print(f"F4_SM_Steps2 @ ({sloc.x:.1f},{sloc.y:.1f},{sloc.z:.1f}) yaw={srot.yaw:.0f}")
    # Player position relative to F4_SM_Steps2 (local frame)
    inv_steps = steps.get_actor_transform().inverse()
    rel = inv_steps.transform_location(player.get_actor_location())
    print(f"  Player in F4_SM_Steps2 local frame: ({rel.x:.1f},{rel.y:.1f},{rel.z:.1f})")
    # Also check F4_SM_Steps2 mesh local bounds (true min/max, not just size)
    smc = steps.get_component_by_class(unreal.StaticMeshComponent)
    if smc and smc.static_mesh:
        lb = smc.get_local_bounds()
        bmin, bmax = lb[0], lb[1]
        print(f"  Mesh local bounds: x=[{bmin.x:.0f},{bmax.x:.0f}] y=[{bmin.y:.0f},{bmax.y:.0f}] z=[{bmin.z:.0f},{bmax.z:.0f}]")

# Capsule overlap at player's exact location and a small neighborhood
def probe(label, dx=0, dy=0, dz=0):
    center = unreal.Vector(ploc.x + dx, ploc.y + dy, ploc.z + dz)
    hits = unreal.SystemLibrary.capsule_overlap_actors(
        gw, center, R, H,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1,
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2],
        None, [player])
    names = []
    for h in (hits or []):
        if h is player:
            continue
        smc = h.get_component_by_class(unreal.StaticMeshComponent)
        m = smc.static_mesh.get_name() if (smc and smc.static_mesh) else "?"
        names.append(f"{h.get_actor_label()}[{m}]")
    coord = f"({center.x:.0f},{center.y:.0f},{center.z:.0f})"
    print(f"  {label:24s} @ {coord}  n={len(names)}: {names[:6]}{(' +'+str(len(names)-6)+' more') if len(names)>6 else ''}")

print("\n--- Capsule overlap probes around player ---")
yaw_rad = math.radians(prot.yaw)
fx, fy = math.cos(yaw_rad), math.sin(yaw_rad)
probe("AT_PLAYER")
probe("FWD_30", dx=30*fx, dy=30*fy)
probe("FWD_60", dx=60*fx, dy=60*fy)
probe("BACK_30", dx=-30*fx, dy=-30*fy)
probe("LEFT_30", dx=30*-fy, dy=30*fx)
probe("RIGHT_30", dx=30*fy, dy=30*-fx)
probe("DOWN_30", dz=-30)
probe("DOWN_50", dz=-50)

# Floor trace down — does floor detection find a surface?
print("\n--- Line trace DOWN (find floor) ---")
start = unreal.Vector(ploc.x, ploc.y, ploc.z + 10)
end   = unreal.Vector(ploc.x, ploc.y, ploc.z - 500)
hr_out = unreal.SystemLibrary.line_trace_single(
    gw, start, end,
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [player], unreal.DrawDebugTrace.NONE, True)
if hr_out:
    hr = hr_out
    hloc = hr.impact_point
    hnorm = hr.impact_normal
    hcomp = hr.component
    hactor = hr.get_editor_property('hit_actor') if False else hr.hit_actor
    print(f"  Floor hit @ ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f}) normal=({hnorm.x:.2f},{hnorm.y:.2f},{hnorm.z:.2f})")
    print(f"  Actor: {hactor.get_actor_label() if hactor else None}")
    print(f"  Component: {hcomp.get_name() if hcomp else None}  Mesh: {(hcomp.static_mesh.get_name() if hasattr(hcomp,'static_mesh') and hcomp.static_mesh else '?')}")
    feet_z = ploc.z - H
    print(f"  Player feet Z={feet_z:.1f}  floor Z={hloc.z:.1f}  gap={feet_z-hloc.z:.1f}")
else:
    print("  NO floor under player")

# Capsule sweep down (matches CMC FindFloor)
print("\n--- Capsule sweep DOWN (CMC FindFloor mimic) ---")
sweep = unreal.SystemLibrary.capsule_trace_single(
    gw, ploc, unreal.Vector(ploc.x, ploc.y, ploc.z - 200), R, H,
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [player], unreal.DrawDebugTrace.NONE, True)
if sweep:
    hloc = sweep.impact_point
    hnorm = sweep.impact_normal
    hcomp = sweep.component
    hactor = sweep.hit_actor
    print(f"  Sweep down hit @ ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f}) normal=({hnorm.x:.2f},{hnorm.y:.2f},{hnorm.z:.2f})")
    print(f"  Actor: {hactor.get_actor_label() if hactor else None}  Comp: {hcomp.get_name() if hcomp else None}")
else:
    print("  Sweep down found nothing")

# Capsule sweep forward (in player's facing direction)
print("\n--- Capsule sweep FORWARD 200 (where they want to move) ---")
end_f = unreal.Vector(ploc.x + 200*fx, ploc.y + 200*fy, ploc.z)
sweep_f = unreal.SystemLibrary.capsule_trace_single(
    gw, ploc, end_f, R, H,
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [player], unreal.DrawDebugTrace.NONE, True)
if sweep_f:
    hloc = sweep_f.impact_point
    hnorm = sweep_f.impact_normal
    hcomp = sweep_f.component
    hactor = sweep_f.hit_actor
    dist = ((hloc.x-ploc.x)**2 + (hloc.y-ploc.y)**2)**0.5
    print(f"  Forward sweep hit at dist {dist:.1f}  @ ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f})")
    print(f"  Normal: ({hnorm.x:.2f},{hnorm.y:.2f},{hnorm.z:.2f})")
    print(f"  Actor: {hactor.get_actor_label() if hactor else None}  Comp: {hcomp.get_name() if hcomp else None}")
    if hcomp and hasattr(hcomp,'static_mesh') and hcomp.static_mesh:
        print(f"  Mesh: {hcomp.static_mesh.get_name()}")
else:
    print("  Forward path clear for 200 UU")

print("\nDone.")
