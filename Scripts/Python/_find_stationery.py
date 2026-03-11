import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
# Search all assets, filter by name
all_assets = ar.get_all_assets()
for a in all_assets:
    name = str(a.asset_name)
    low = name.lower()
    if "stationery" in low or "polysurface" in low or "f5_" in low or "seperat" in low:
        unreal.log(str(a.package_name) + " -> " + name)
