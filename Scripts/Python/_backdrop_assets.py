import unreal, json
ar = unreal.AssetRegistryHelpers.get_asset_registry()
print("=== /Game/BackgroundBuildings contents ===")
for ad in ar.get_assets_by_path("/Game/BackgroundBuildings", recursive=True):
    print("  %-26s %s" % (ad.asset_class_path.asset_name, ad.package_name))
