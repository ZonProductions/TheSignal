import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

WW = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_WindowWall.SM_WindowWall")
b = WW.get_bounds().box_extent
print("SM_WindowWall extent (UU):", round(b.x,1), round(b.y,1), round(b.z,1))
print("SM_WindowWall materials:")
for i in range(WW.get_num_sections(0)):
    pass
si = WW.static_materials
for idx, m in enumerate(si):
    mi = m.material_interface
    print("   slot %d: %s" % (idx, mi.get_name() if mi else "(none)"))

# sample some window wall actors: transform + per-slot mat
print("\n=== sample SM_WindowWall actors ===")
n = 0
scales = {}
for a in eas.get_all_level_actors():
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm or sm.get_name() != "SM_WindowWall":
        continue
    s = a.get_actor_scale3d()
    key = (round(s.x,2), round(s.y,2), round(s.z,2))
    scales[key] = scales.get(key, 0) + 1
    if n < 6:
        loc = a.get_actor_location()
        rot = a.get_actor_rotation()
        c = comps[0]
        mats = [ (c.get_material(i).get_name() if c.get_material(i) else "none") for i in range(c.get_num_materials()) ]
        print("  %-26s loc=(%.0f,%.0f,%.0f) yaw=%.0f scale=%s mats=%s" % (
            a.get_actor_label(), loc.x, loc.y, loc.z, rot.yaw, key, mats))
        n += 1
print("scale variants among window walls:", scales)
