import unreal

# Fixes walk-through / fall-through meshes from purchased packs:
# any blocking StaticMeshActor whose mesh has NO simple collision primitives
# (convex/box/sphere/capsule) AND isn't already complex-as-simple.
# The player capsule uses SIMPLE collision for movement -> no simple geo = fall through.
# Switching to CTF_USE_COMPLEX_AS_SIMPLE makes the capsule collide against the render mesh.

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

def simple_count(agg):
    if not agg:
        return 0
    n = 0
    for prop in ('convex_elems', 'box_elems', 'sphere_elems', 'sphyl_elems', 'tapered_capsule_elems'):
        try:
            n += len(agg.get_editor_property(prop))
        except Exception:
            pass
    return n

CAS = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
fixed = set()
skipped_no_block = 0
print("=== MESHES WITH NO SIMPLE COLLISION (fall/walk-through) ===")
for a in actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        mesh = c.static_mesh
        if not mesh:
            continue
        p = mesh.get_path_name()
        if p in fixed:
            continue
        body = mesh.get_editor_property('body_setup')
        if not body:
            continue
        flag = body.get_editor_property('collision_trace_flag')
        if flag == CAS:
            continue  # already collides via render geo
        agg = body.get_editor_property('agg_geom')
        if simple_count(agg) > 0:
            continue  # has real simple collision, leave it alone
        # No simple collision. Only matters if the component actually blocks.
        ce = c.get_collision_enabled()
        if ce == unreal.CollisionEnabled.NO_COLLISION:
            skipped_no_block += 1
            continue
        bounds = c.get_local_bounds()
        bmin, bmax = bounds[0], bounds[1]
        mx = max(bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z)
        print(f"  {mesh.get_name()} (e.g. {a.get_actor_label()}) size~{mx:.0f}")
        body.set_editor_property('collision_trace_flag', CAS)
        ok = unreal.EditorAssetLibrary.save_asset(p)
        print(f"    FIXED -> complex-as-simple, saved={ok}")
        fixed.add(p)

print(f"\nFixed {len(fixed)} mesh assets (no-collision -> complex-as-simple)")
print(f"(left {skipped_no_block} no-collision components that don't block)")
