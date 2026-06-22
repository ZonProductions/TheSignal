import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

print("=== sample F3_SM_wallBrick actors (the regular wall) ===")
n = 0
matcount = {}
thick = {}
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if "wallbrick" not in lbl.lower():
        continue
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm or sm.get_name() != "SM_Cube":
        continue
    org, ext = a.get_actor_bounds(False)
    s = a.get_actor_scale3d()
    mats = []
    for i in range(c.get_num_materials()):
        m = c.get_material(i)
        mats.append(m.get_name() if m else "none")
        if m: matcount[m.get_name()] = matcount.get(m.get_name(), 0) + 1
    # thinnest world axis = wall thickness
    t = min(ext.x, ext.y, ext.z) * 2
    thick[round(t)] = thick.get(round(t), 0) + 1
    if n < 8:
        print("  %-22s ext=(%.0f,%.0f,%.0f) scale=(%.2f,%.2f,%.2f) mats=%s" % (
            lbl, ext.x*2, ext.y*2, ext.z*2, s.x, s.y, s.z, mats))
        n += 1
print("material tally on wallBrick cubes:", matcount)
print("wall thickness (full UU) tally:", dict(sorted(thick.items())))

# resolve MI_WallWhite full path from a window wall slot 2
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm = cs[0].get_editor_property("static_mesh")
    if sm and sm.get_name() == "SM_WindowWall":
        m = cs[0].get_material(2)
        print("window wall slot2 material path:", m.get_path_name() if m else None)
        break
