import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","World")):
    n=str(ad.asset_name)
    if "building1" in n.lower() or "bigcompany" in n.lower():
        print(" ", ad.package_name)
