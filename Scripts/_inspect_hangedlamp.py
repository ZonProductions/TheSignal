import unreal

# Find the asset
ar = unreal.AssetRegistryHelpers.get_asset_registry()
hits = []
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh"), True):
    n = str(ad.asset_name)
    if "HangedLamp" in n or "HangedWire" in n:
        hits.append(str(ad.package_name))
print("MATCHING STATIC MESHES:", hits)

sm = None
for p in hits:
    if "HangedLamp" in p:
        sm = unreal.load_asset(p); break
if sm:
    print("LOADED:", sm.get_path_name())
    b = sm.get_bounds()
    org = b.origin; ext = b.box_extent
    print("BOUNDS origin:", org, "extent:", ext)
    print("Z range: min %.2f  max %.2f  (height %.2f)" % (org.z - ext.z, org.z + ext.z, ext.z * 2))
    print("X half-width %.2f  Y half-width %.2f" % (ext.x, ext.y))
    print("Num materials:", sm.get_num_sections(0), "| LODs:", sm.get_num_lods())
    socks = sm.get_sockets()
    for s in socks:
        print("SOCKET:", s.socket_name, "loc:", s.relative_location)

# Geometry Script availability
gs_ok = hasattr(unreal, "GeometryScript_StaticMesh") or hasattr(unreal, "GeometryScript_MeshModeling")
print("GeometryScript modeling available:", hasattr(unreal, "GeometryScript_MeshModeling"))
print("GeometryScript staticmesh available:", hasattr(unreal, "GeometryScript_StaticMesh"))
