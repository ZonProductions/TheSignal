import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/UMGEditor","WidgetBlueprint")):
    n=str(ad.asset_name)
    if "hud" in n.lower():
        print(n, "->", ad.package_name)
