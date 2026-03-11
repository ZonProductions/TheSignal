import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = ar.get_all_assets()
for a in all_assets:
    name = str(a.asset_name)
    if "polySurface" in name or "F5_" in name or "f5_" in name.lower():
        unreal.log(str(a.package_name) + " -> " + name + " (" + str(a.asset_class_path) + ")")
