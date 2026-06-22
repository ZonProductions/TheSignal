import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def hits(cls, keys):
    out=[]
    for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", cls)):
        nm=str(ad.asset_name).lower()
        if any(k in nm for k in keys):
            out.append(str(ad.package_name))
    return out

print("=== GRASS/LAWN materials ===")
for p in sorted(set(hits("Material",["grass","lawn","field"])+hits("MaterialInstanceConstant",["grass","lawn","field"]))): print("  ",p)
print("=== CONCRETE/ASPHALT/PAVEMENT materials ===")
for p in sorted(set(hits("Material",["concrete","asphalt","pavement","ground","dirt"])+hits("MaterialInstanceConstant",["concrete","asphalt","pavement","ground","dirt"]))): print("  ",p)
print("=== TREE / FOLIAGE / PLANT static meshes ===")
for p in sorted(set(hits("StaticMesh",["tree","pine","fir","oak","foliage","plant","bush","trunk","leaf","forest"]))): print("  ",p)

# building base Z + footprint (exclude degenerate + the backdrop we added)
xs=[];ys=[];zmins=[]
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Backdrop_Building_"): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs or not cs[0].get_editor_property("static_mesh"): continue
    org,ext=a.get_actor_bounds(False)
    if ext.x>50000 or ext.y>50000: continue
    if abs(org.x)>30000 or abs(org.y)>30000: continue   # ignore stray far props
    xs+=[org.x-ext.x,org.x+ext.x]; ys+=[org.y-ext.y,org.y+ext.y]; zmins.append(org.z-ext.z)
print("\nBUILDING footprint X[%.0f..%.0f] Y[%.0f..%.0f]  base_z_min=%.0f"%(min(xs),max(xs),min(ys),max(ys),min(zmins)))
