import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 1) overall XY footprint of remaining (floor-3) geometry
xs=[]; ys=[]; zs=[]
for a in eas.get_all_level_actors():
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    if not cs[0].get_editor_property("static_mesh"): continue
    org,ext=a.get_actor_bounds(False)
    xs+= [org.x-ext.x, org.x+ext.x]; ys+=[org.y-ext.y, org.y+ext.y]; zs+=[org.z]
print("floor3 footprint X[%.0f..%.0f] Y[%.0f..%.0f]  (size %.0f x %.0f)" % (
    min(xs),max(xs),min(ys),max(ys), max(xs)-min(xs), max(ys)-min(ys)))
print("geometry Z median ~", sorted(zs)[len(zs)//2])

# 2) remaining EXTERIOR windows (glass SM_Cube panels + exit doors) with facing (thin axis)
print("\n=== exterior glass panels / doors (loc, thin-axis=facing) ===")
for a in eas.get_all_level_actors():
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c=cs[0]; sm=c.get_editor_property("static_mesh")
    if not sm: continue
    mats=[c.get_material(i).get_name() if c.get_material(i) else "" for i in range(c.get_num_materials())]
    if sm.get_name() not in ("SM_Cube","SM_DoorExit"): continue
    if not any("Glass" in m for m in mats): continue
    org,ext=a.get_actor_bounds(False); loc=a.get_actor_location()
    thin = "X(faces +/-X)" if ext.x<ext.y and ext.x<ext.z else ("Y(faces +/-Y)" if ext.y<ext.x and ext.y<ext.z else "Z")
    print("  %-22s %-12s loc=(%.0f,%.0f,%.0f) ext=(%.0f,%.0f,%.0f) facing=%s" % (
        a.get_actor_label(), sm.get_name(), loc.x,loc.y,loc.z, ext.x*2,ext.y*2,ext.z*2, thin))

# 3) B_LowPolyBuilding bounds/orientation
bp = unreal.load_asset("/Game/BackgroundBuildings/Blueprint/B_LowPolyBuilding")
print("\nB_LowPolyBuilding loaded:", bool(bp))
