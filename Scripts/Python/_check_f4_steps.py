"""Diagnose F4_SM_Steps walkability problem.

Two suspects:
  (a) The mesh asset itself has a single convex hull (= invisible shell over
      the stairs). Fix: CTF_USE_COMPLEX_AS_SIMPLE on the asset's BodySetup.
  (b) A separate actor/component is blocking the stairwell volume.

Run from UE5 editor Python console:
  py Scripts/Python/_check_f4_steps.py
"""
import unreal

TARGET_LABEL_SUBSTR = "F4_SM_Steps"

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

targets = [a for a in actors if TARGET_LABEL_SUBSTR.lower() in a.get_actor_label().lower()]

print(f"=== Found {len(targets)} actor(s) matching '{TARGET_LABEL_SUBSTR}' ===")
if not targets:
    # Fallback: any actor whose mesh is SM_Steps
    for a in actors:
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            if m and "SM_Steps" in m.get_name():
                targets.append(a)
                break
    print(f"  Fallback by mesh name: {len(targets)} actor(s)")

for a in targets:
    label = a.get_actor_label()
    loc = a.get_actor_location()
    print(f"\n--- {label} @ ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) ---")

    # List ALL primitive components on this actor (catches extra collision shells)
    prims = a.get_components_by_class(unreal.PrimitiveComponent)
    print(f"  Primitive components: {len(prims)}")
    for c in prims:
        cname = c.get_name()
        ctype = type(c).__name__
        cenabled = c.get_collision_enabled()
        cprofile = c.get_collision_profile_name()
        try:
            rel = c.relative_location
            rel_str = f"({rel.x:.0f},{rel.y:.0f},{rel.z:.0f})"
        except Exception:
            rel_str = "n/a"
        print(f"    [{ctype}] {cname} collision={cenabled} profile={cprofile} relLoc={rel_str}")

    # Inspect the mesh asset(s) referenced
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        mesh = c.static_mesh
        if not mesh:
            continue
        body = mesh.get_editor_property('body_setup')
        if not body:
            print(f"  Mesh {mesh.get_name()}: NO BODY SETUP")
            continue
        flag = body.get_editor_property('collision_trace_flag')
        agg = body.get_editor_property('agg_geom')
        n_convex = len(agg.get_editor_property('convex_elems')) if agg else 0
        n_boxes = len(agg.get_editor_property('box_elems')) if agg else 0
        n_spheres = len(agg.get_editor_property('sphere_elems')) if agg else 0
        n_capsules = len(agg.get_editor_property('sphyl_elems')) if agg else 0

        bounds = c.get_local_bounds()
        sx = bounds[1].x - bounds[0].x
        sy = bounds[1].y - bounds[0].y
        sz = bounds[1].z - bounds[0].z

        verdict = "OK (complex-as-simple)" if flag == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE else "SUSPECT"
        print(f"  Mesh: {mesh.get_path_name()}")
        print(f"    size: {sx:.0f} x {sy:.0f} x {sz:.0f}")
        print(f"    trace_flag: {flag}  --> {verdict}")
        print(f"    primitives: convex={n_convex} box={n_boxes} sphere={n_spheres} capsule={n_capsules}")
        if n_convex > 0 and flag != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
            print(f"    >>> SHELL SUSPECT: {n_convex} convex hull(s) with default trace flag <<<")

    # Look for nearby blocking actors within a stairwell-sized radius
    ext = a.get_actor_bounds(only_colliding_components=False)
    origin, box_extent = ext
    radius = max(box_extent.x, box_extent.y, box_extent.z) * 1.5 + 200
    print(f"  Scanning for nearby blocking actors within {radius:.0f} UU of origin ({origin.x:.0f},{origin.y:.0f},{origin.z:.0f}):")
    for b in actors:
        if b is a:
            continue
        bloc = b.get_actor_location()
        d = ((bloc.x - origin.x) ** 2 + (bloc.y - origin.y) ** 2 + (bloc.z - origin.z) ** 2) ** 0.5
        if d > radius:
            continue
        # Only flag actors with any blocking primitive
        for c in b.get_components_by_class(unreal.PrimitiveComponent):
            ce = c.get_collision_enabled()
            if ce in (
                unreal.CollisionEnabled.QUERY_AND_PHYSICS,
                unreal.CollisionEnabled.QUERY_ONLY,
            ):
                cp = c.get_collision_profile_name()
                if "BlockAll" in str(cp) or "Block" in str(cp):
                    print(f"    NEARBY BLOCKER: {b.get_actor_label()} [{type(b).__name__}] dist={d:.0f} comp={c.get_name()} profile={cp}")
                    break

print("\nDone.")
