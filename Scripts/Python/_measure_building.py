import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# clean footprint: ignore actors with absurd bounds (sky sphere etc.)
xs=[]; ys=[]
for a in eas.get_all_level_actors():
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    if not cs[0].get_editor_property("static_mesh"): continue
    org,ext=a.get_actor_bounds(False)
    if ext.x>50000 or ext.y>50000 or ext.z>50000: continue   # skip degenerate
    xs+=[org.x-ext.x, org.x+ext.x]; ys+=[org.y-ext.y, org.y+ext.y]
print("CLEAN floor3 footprint X[%.0f..%.0f] Y[%.0f..%.0f] center=(%.0f,%.0f)" % (
    min(xs),max(xs),min(ys),max(ys),(min(xs)+max(xs))/2,(min(ys)+max(ys))/2))

# spawn one B_LowPolyBuilding far away to measure its size + see its default look
bp = unreal.load_asset("/Game/BackgroundBuildings/Blueprint/B_LowPolyBuilding")
loc = unreal.Vector(0,0,-50000)
act = eas.spawn_actor_from_object(bp, loc)
org,ext = act.get_actor_bounds(False)
print("B_LowPolyBuilding size = (%.0f x %.0f x %.0f)  pivotZoffsetFromCenter=%.0f" % (
    ext.x*2, ext.y*2, ext.z*2, org.z-loc.z))
# list its exposed properties that look like size/height/variant
for p in ["BuildingHeight","Height","Floors","NumFloors","BuildingType","Variant","Width","Seed"]:
    try:
        v=act.get_editor_property(p); print("   prop", p, "=", v)
    except Exception: pass
eas.destroy_actor(act)
print("measured + cleaned up")
