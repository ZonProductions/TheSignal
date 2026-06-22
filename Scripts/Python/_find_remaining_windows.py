import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

GLASS = ("MI_Glass","MI_GlassBlury","MI_GlassDark","MI_Glass_AO")
print("=== actors using exterior-type glass (mesh | mat | loc | footprint) ===")
seen = {}
for a in eas.get_all_level_actors():
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c = cs[0]
    sm = c.get_editor_property("static_mesh")
    if not sm: continue
    mats = [c.get_material(i).get_name() if c.get_material(i) else "" for i in range(c.get_num_materials())]
    if not any(g in mats for g in GLASS): continue
    if sm.get_name() == "SM_FireExtinguisherBoxGlass": continue
    loc = a.get_actor_location()
    org, ext = a.get_actor_bounds(False)
    key = sm.get_name()
    seen[key] = seen.get(key,0)+1
    if seen[key] <= 4:
        print("  %-26s mesh=%-22s loc=(%.0f,%.0f,%.0f) ext=(%.0f,%.0f,%.0f) mats=%s" % (
            a.get_actor_label(), sm.get_name(), loc.x, loc.y, loc.z, ext.x*2, ext.y*2, ext.z*2,
            [m for m in mats if m]))
print("\nglass-actor mesh tally:", seen)

# Where are the LightmassPortals (mark window openings)?
print("\n=== LightmassPortal locations ===")
n=0
for a in eas.get_all_level_actors():
    if a.get_class().get_name()=="LightmassPortal":
        loc=a.get_actor_location()
        if n<8: print("  portal loc=(%.0f,%.0f,%.0f)"%(loc.x,loc.y,loc.z))
        n+=1
print("  total portals:", n)
