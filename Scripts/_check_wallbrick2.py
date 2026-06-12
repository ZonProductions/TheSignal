import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in sub.get_all_level_actors():
    if 'wallBrick' in a.get_actor_label():
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            m = c.static_mesh
            if not m:
                continue
            body = m.get_editor_property('body_setup')
            agg = body.get_editor_property('agg_geom') if body else None
            flag = body.get_editor_property('collision_trace_flag') if body else None
            nconvex = len(agg.get_editor_property('convex_elems')) if agg else -1
            nbox = len(agg.get_editor_property('box_elems')) if agg else -1
            b = m.get_bounds().box_extent
            unreal.log(f'actor={a.get_actor_label()} mesh={m.get_path_name()}')
            unreal.log(f'  flag={flag} convex={nconvex} box={nbox} extents=({b.x:.0f},{b.y:.0f},{b.z:.0f})')
        break
