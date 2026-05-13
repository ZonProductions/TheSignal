"""Capsule overlap at the player's current PIE location.

Grabs the live PIE pawn, reads its capsule + world position, then runs a
capsule overlap to enumerate every actor/component the player capsule is
currently touching. Goal: identify the "shell or block" the player feels.
"""
import unreal

# Get the PIE pawn. In UE 5.4, GameplayStatics with world_context=None
# resolves to the active game world when invoked from Python during PIE.
pp = unreal.GameplayStatics.get_player_pawn(None, 0)
if not pp:
    # Fallback: iterate any Pawn we can see
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in sub.get_all_level_actors():
        if isinstance(a, unreal.Pawn):
            pp = a
            break
if not pp:
    print("No PIE pawn found. Confirm you're in PIE and try again.")
    raise SystemExit

ploc = pp.get_actor_location()
prot = pp.get_actor_rotation()
print(f"PLAYER @ ({ploc.x:.0f},{ploc.y:.0f},{ploc.z:.0f}) yaw={prot.yaw:.0f}")
print(f"Class: {pp.get_class().get_name()}  Label: {pp.get_actor_label()}")

# Read player's actual capsule
cap = pp.get_component_by_class(unreal.CapsuleComponent)
if cap:
    R = cap.get_scaled_capsule_radius()
    H = cap.get_scaled_capsule_half_height()
    print(f"Player capsule: r={R:.1f} half_height={H:.1f}")
else:
    R, H = 34.0, 88.0
    print(f"No capsule on pawn — using defaults r={R} hh={H}")

world = pp.get_world()

# Capsule overlap exactly where the player stands (and small probes around)
def probe(label, dx=0, dy=0, dz=0):
    center = unreal.Vector(ploc.x + dx, ploc.y + dy, ploc.z + dz)
    hits = unreal.SystemLibrary.capsule_overlap_actors(
        world, center, R, H,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1,  # WorldStatic
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2,  # WorldDynamic
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY4,  # PhysicsBody
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY5,  # Vehicle
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY6], # Destructible
        None, [pp], [])
    coord = f"({center.x:.0f},{center.y:.0f},{center.z:.0f})"
    names = [h.get_actor_label() for h in (hits or [])]
    print(f"  {label:24s} @ {coord}  hits={len(names)}: {names}")

print("\n--- Capsule overlap probes ---")
probe("AT_PLAYER")
probe("FORWARD_60",   dx=60 *  __import__('math').cos(__import__('math').radians(prot.yaw)),
                      dy=60 *  __import__('math').sin(__import__('math').radians(prot.yaw)))
probe("FORWARD_120",  dx=120 * __import__('math').cos(__import__('math').radians(prot.yaw)),
                      dy=120 * __import__('math').sin(__import__('math').radians(prot.yaw)))
probe("DOWN_50",      dz=-50)
probe("DOWN_100",     dz=-100)
probe("UP_50",        dz=50)

# Also: a precise line trace straight down to find the floor surface, and the
# capsule sweep DOWN (matches what CharacterMovementComponent's FindFloor does)
print("\n--- Line trace DOWN from player center to find floor ---")
start = unreal.Vector(ploc.x, ploc.y, ploc.z + 10)
end   = unreal.Vector(ploc.x, ploc.y, ploc.z - 500)
hit = unreal.SystemLibrary.line_trace_single(
    world, start, end,
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [pp],
    unreal.DrawDebugTrace.NONE,
    True)
if hit:
    h = hit
    # In 5.4 line_trace_single returns HitResult struct directly
    try:
        hloc = h.impact_point
        hnorm = h.impact_normal
        hcomp = h.component
        hactor = h.actor
        print(f"  Floor at ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f}) normal=({hnorm.x:.2f},{hnorm.y:.2f},{hnorm.z:.2f})")
        print(f"  Actor: {hactor.get_actor_label() if hactor else None}   Component: {hcomp.get_name() if hcomp else None}")
        print(f"  Drop below player feet: {(ploc.z - H) - hloc.z:.1f} UU")
    except Exception as e:
        print(f"  hit struct: {h}  err: {e}")
else:
    print("  No floor under player (!)")

# Now sweep capsule straight down (matches CMC FindFloor)
print("\n--- Capsule sweep DOWN (mimics FindFloor) ---")
sweep_start = unreal.Vector(ploc.x, ploc.y, ploc.z)
sweep_end   = unreal.Vector(ploc.x, ploc.y, ploc.z - 200)
sweep_hit = unreal.SystemLibrary.capsule_trace_single(
    world, sweep_start, sweep_end, R, H,
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [pp],
    unreal.DrawDebugTrace.NONE,
    True)
if sweep_hit:
    try:
        hloc = sweep_hit.impact_point
        hcomp = sweep_hit.component
        hactor = sweep_hit.actor
        print(f"  Sweep hit at ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f})")
        print(f"  Actor: {hactor.get_actor_label() if hactor else None}   Component: {hcomp.get_name() if hcomp else None}")
    except Exception as e:
        print(f"  sweep hit: {sweep_hit}  err: {e}")
else:
    print("  Capsule sweep down found no floor")

print("\nDone.")
