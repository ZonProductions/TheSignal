import unreal

found = unreal.EditorAssetLibrary.list_assets('/Game', recursive=True, include_folder=False)
hits = [a for a in found if 'wallbrick' in a.lower() and 'BACKUP' not in a]
unreal.log(f'{len(hits)} wallBrick assets')
for h in hits[:10]:
    m = unreal.load_asset(h)
    if not isinstance(m, unreal.StaticMesh):
        continue
    body = m.get_editor_property('body_setup')
    if not body:
        unreal.log(f'{m.get_name()}: NO bodysetup')
        continue
    agg = body.get_editor_property('agg_geom')
    flag = body.get_editor_property('collision_trace_flag')
    nconvex = len(agg.get_editor_property('convex_elems'))
    nbox = len(agg.get_editor_property('box_elems'))
    nsphere = len(agg.get_editor_property('sphere_elems'))
    nsphyl = len(agg.get_editor_property('sphyl_elems'))
    b = m.get_bounds().box_extent
    unreal.log(f'{m.get_name()}: flag={flag} convex={nconvex} box={nbox} sphere={nsphere} sphyl={nsphyl} extents=({b.x:.0f},{b.y:.0f},{b.z:.0f})')
