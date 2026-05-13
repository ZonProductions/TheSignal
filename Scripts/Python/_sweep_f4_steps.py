"""Capsule sweep along F4_SM_Steps2 walkable path.

The mesh is L-shape stairs (one ramp -> landing -> other ramp turning 90deg).
Box primitives in local space told us:
  Box[0] center=(-143, 74, 239) rot pitch=+30  -> upper ramp
  Box[1] center=(-153, -70, 90) rot pitch=-30  -> lower ramp
  Box[2] center=(-341, 10, 161) rot=0           -> landing (PHANTOM — outside visible mesh)

Sweep the capsule (Grace default-ish: r=34, h=88) along an estimated walk path
and log every hit.

NOTE: Box[2] sitting at local X=-341 (outside the visible -191 mesh edge) is
the prime suspect. With CTF_USE_COMPLEX_AS_SIMPLE this *should* be ignored,
but a capsule sweep using the default trace channel will confirm.
"""
import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = sub.get_all_level_actors()
print(f"Editor world initially: {len(all_actors)} actors")
if not all_actors:
    # Open the map first
    print("Loading Maps_BigCompany...")
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/office_BigCompanyArchViz/Maps/Maps_BigCompany")
    all_actors = sub.get_all_level_actors()
    print(f"After load: {len(all_actors)} actors")
target = None
matches = [a for a in all_actors if "F4_SM_Steps2" in a.get_actor_label()]
print(f"F4_SM_Steps2 substring matches: {len(matches)} -> {[a.get_actor_label() for a in matches]}")
if matches:
    target = matches[0]
else:
    # Maybe label changed; find by mesh asset
    for a in all_actors:
        smc = a.get_component_by_class(unreal.StaticMeshComponent) if hasattr(a, 'get_component_by_class') else None
        if smc and smc.static_mesh and smc.static_mesh.get_name() == "SM_Steps":
            lbl = a.get_actor_label()
            if "F4" in lbl:
                print(f"Fallback match by mesh: {lbl}")
                target = a
                break
if not target:
    raise SystemExit("No F4 SM_Steps actor found")

aloc = target.get_actor_location()
arot = target.get_actor_rotation()
print(f"Target @ ({aloc.x:.0f},{aloc.y:.0f},{aloc.z:.0f}) rot=({arot.pitch:.0f},{arot.yaw:.0f},{arot.roll:.0f})")

# World transform helper
def local_to_world(lx, ly, lz):
    p = unreal.Vector(lx, ly, lz)
    # Use the actor's TransformLocation
    return target.get_actor_transform().transform_location(p)

# Capsule
RADIUS = 34.0
HALF_HEIGHT = 88.0

# Define waypoints in LOCAL mesh space.
# Mesh local bounds X: [-191, +191], Y: [-140, +140], Z: [-189, +189]
# Box[0] upper ramp local center=(-143, 74, 239)  — but wait, mesh bounds Z up to 189.
#   Box center Z=239 means box sticks 50UU above mesh top. Maybe in local pre-pivot space.
# Better: sample world-space points along the stair surface.
# Take Z range from world bounds: floor of stairs ~ 1499, top ~ 2065. Walk top->bottom.

# I'll define a synthetic walk path in WORLD space along the stair bounds.
# Top of upper ramp (player coming from upper floor), step across landing, down lower ramp.
# Without the exact mesh layout I'll do a coarse vertical sweep at multiple X positions
# inside the L-shape, plus a sweep AT THE PHANTOM BOX[2] LOCATION specifically.

waypoints_local = [
    # (local_x, local_y, local_z, label)
    # Upper ramp surface samples (approx along Box[0])
    (   0,   74, 280, "upper_ramp_top"),
    ( -50,   74, 240, "upper_ramp_mid"),
    (-100,   74, 200, "upper_ramp_low"),
    # Landing center (where Box[2] is — but outside visible mesh!)
    (-150,   10, 165, "landing_entry"),
    (-200,   10, 165, "landing_mid"),
    (-280,   10, 165, "landing_phantom_zone"),   # Inside Box[2]'s X range
    (-330,   10, 165, "landing_phantom_center"), # AT Box[2] center
    (-150,  -70,  90, "lower_ramp_top"),
    (-100,  -70,  70, "lower_ramp_mid"),
    ( -50,  -70,  50, "lower_ramp_bot"),
]

# Run overlap_multi_by_object at each waypoint to find what the capsule hits
ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ews.get_editor_world()

print(f"\nCapsule overlap tests (r={RADIUS}, hh={HALF_HEIGHT}):")
print(f"{'label':28s} {'world':32s} {'#hits':>6s}  hits")
for lx, ly, lz, label in waypoints_local:
    w = local_to_world(lx, ly, lz)
    # Lift capsule center so its bottom is at the surface point (capsule center = surface_z + half_height)
    center = unreal.Vector(w.x, w.y, w.z + HALF_HEIGHT)
    hits = unreal.SystemLibrary.capsule_overlap_actors(
        world, center, RADIUS, HALF_HEIGHT,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1,  # WorldStatic
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2,  # WorldDynamic
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY3], # Pawn (sanity)
        None, [target], [])
    coord = f"({w.x:.0f},{w.y:.0f},{w.z:.0f})"
    hit_labels = [h.get_actor_label() for h in (hits or []) if h is not target]
    extra = ""
    if hit_labels:
        extra = " <-- " + ", ".join(hit_labels[:5])
        if len(hit_labels) > 5:
            extra += f" +{len(hit_labels)-5} more"
    print(f"{label:28s} {coord:32s} {len(hit_labels):>6d}{extra}")

print("\nDone.")
