import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
keys = ["pine","fir","redwood","spruce","cedar","conifer","evergreen","tree","hemlock","douglas","trunk","sapling","stump"]
print("=== StaticMesh tree-ish assets ===")
found=[]
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","StaticMesh")):
    nm=str(ad.asset_name).lower()
    if any(k in nm for k in keys) and "street" not in nm and "preview" not in nm:
        found.append(str(ad.package_name))
for p in sorted(found): print("  ",p)
print("total:", len(found))
