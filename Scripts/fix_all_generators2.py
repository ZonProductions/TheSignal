import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

# Find ALL large meshes that could have bad convex hull collision
# Don't filter by name — scan everything
print("=== ALL STATIC MESH ACTORS WITH SINGLE CONVEX HULL COLLISION ===")
fixed_meshes = set()
for a in actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        mesh = c.static_mesh
        if not mesh or mesh.get_path_name() in fixed_meshes:
            continue
        body = mesh.get_editor_property('body_setup')
        if not body:
            continue
        agg = body.get_editor_property('agg_geom')
        if not agg:
            continue
        convex = agg.get_editor_property('convex_elems')
        flag = body.get_editor_property('collision_trace_flag')
        # Find meshes with convex hulls that aren't already fixed
        if len(convex) > 0 and flag != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
            label = a.get_actor_label()
            bounds = c.get_local_bounds()
            bmin = bounds[0]
            bmax = bounds[1]
            sx = bmax.x - bmin.x
            sy = bmax.y - bmin.y
            sz = bmax.z - bmin.z
            # Only fix large meshes (>300 UU in any dimension) — small props are fine with convex
            if max(sx, sy, sz) > 300:
                print(f"  {label}: {mesh.get_name()} — {len(convex)} convex, size ({sx:.0f}x{sy:.0f}x{sz:.0f})")
                body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
                unreal.EditorAssetLibrary.save_asset(mesh.get_path_name())
                print(f"    FIXED -> complex-as-simple, saved")
                fixed_meshes.add(mesh.get_path_name())

print(f"\nFixed {len(fixed_meshes)} mesh assets")

# Also list what was already fixed
print("\n=== ALREADY USING COMPLEX-AS-SIMPLE ===")
already = set()
for a in actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    for c in comps:
        mesh = c.static_mesh
        if not mesh or mesh.get_path_name() in already:
            continue
        body = mesh.get_editor_property('body_setup')
        if not body:
            continue
        flag = body.get_editor_property('collision_trace_flag')
        if flag == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
            label = a.get_actor_label()
            print(f"  {mesh.get_name()} (e.g. {label})")
            already.add(mesh.get_path_name())
