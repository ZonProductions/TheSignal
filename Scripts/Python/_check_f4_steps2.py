"""Focused F4_SM_Steps diagnosis (round 2).

We confirmed trace_flag is correct — so capsule sweeps should hit the mesh.
But 3 box primitives exist on the BodySetup; if any wrap the turn region,
some collision paths (overlap queries, stepping logic) may still hit them.

Also do a real OVERLAP test against each F4_SM_Steps actor's world bounds
to find any actor that physically intrudes on the stair volume.
"""
import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

targets = [a for a in actors if "F4_SM_Steps" in a.get_actor_label()]
print(f"=== {len(targets)} F4_SM_Steps actor(s) ===")

for a in targets:
    label = a.get_actor_label()
    aloc = a.get_actor_location()
    arot = a.get_actor_rotation()
    print(f"\n=== {label} @ ({aloc.x:.0f},{aloc.y:.0f},{aloc.z:.0f}) rot=({arot.pitch:.0f},{arot.yaw:.0f},{arot.roll:.0f}) ===")

    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    mesh = smc.static_mesh

    # World bounds of the actor (the "stair volume" we care about)
    origin, extent = a.get_actor_bounds(only_colliding_components=True)
    print(f"World bounds: origin=({origin.x:.0f},{origin.y:.0f},{origin.z:.0f}) extent=({extent.x:.0f},{extent.y:.0f},{extent.z:.0f})")
    print(f"  => box mins ({origin.x-extent.x:.0f},{origin.y-extent.y:.0f},{origin.z-extent.z:.0f})  maxs ({origin.x+extent.x:.0f},{origin.y+extent.y:.0f},{origin.z+extent.z:.0f})")

    # Dump the actual collision primitives on the BodySetup
    body = mesh.get_editor_property('body_setup')
    if body:
        agg = body.get_editor_property('agg_geom')
        boxes = agg.get_editor_property('box_elems')
        print(f"BodySetup box primitives: {len(boxes)}")
        for i, b in enumerate(boxes):
            center = b.get_editor_property('center')
            rot = b.get_editor_property('rotation')
            x = b.get_editor_property('x')
            y = b.get_editor_property('y')
            z = b.get_editor_property('z')
            print(f"  Box[{i}]: center=({center.x:.0f},{center.y:.0f},{center.z:.0f}) size=({x:.0f},{y:.0f},{z:.0f}) rot=({rot.pitch:.0f},{rot.yaw:.0f},{rot.roll:.0f})")
            # Flag: is this box obviously larger than half a stair step?
            # SM_Steps mesh is 382x279x377 — a single box covering most of that = SHELL
            if x > 200 or y > 200 or z > 200:
                print(f"    >>> LARGE BOX (>{200}UU on some axis) — could act as shell <<<")

    # Find any OTHER actor whose bounds intersect this actor's bounds
    print("Overlapping actors (bounds intersect, excluding self & floor tiles):")
    found_overlap = False
    for b in actors:
        if b is a:
            continue
        blabel = b.get_actor_label()
        # Skip floor tiles & this floor's wall pieces — they're expected to touch the stairs
        if "FloorTile" in blabel:
            continue
        try:
            borigin, bextent = b.get_actor_bounds(only_colliding_components=True)
        except Exception:
            continue
        # AABB overlap test
        if (abs(borigin.x - origin.x) < (extent.x + bextent.x)
            and abs(borigin.y - origin.y) < (extent.y + bextent.y)
            and abs(borigin.z - origin.z) < (extent.z + bextent.z)):
            # Check if it blocks the player (Pawn channel)
            primb = b.get_component_by_class(unreal.PrimitiveComponent)
            if primb:
                ce = primb.get_collision_enabled()
                if ce in (unreal.CollisionEnabled.QUERY_AND_PHYSICS, unreal.CollisionEnabled.QUERY_ONLY):
                    cp = primb.get_collision_profile_name()
                    # Penetration depth (positive = overlapping)
                    px = (extent.x + bextent.x) - abs(borigin.x - origin.x)
                    py = (extent.y + bextent.y) - abs(borigin.y - origin.y)
                    pz = (extent.z + bextent.z) - abs(borigin.z - origin.z)
                    print(f"  {blabel} [{type(b).__name__}] profile={cp} penetration=({px:.0f},{py:.0f},{pz:.0f})")
                    found_overlap = True
    if not found_overlap:
        print("  (none)")

print("\nDone.")
