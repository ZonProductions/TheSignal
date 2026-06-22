import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
print("=== PN_interactiveSpruceForest static meshes + sizes ===")
for ad in ar.get_assets_by_path("/Game/PN_interactiveSpruceForest", recursive=True):
    if str(ad.asset_class_path.asset_name)=="StaticMesh":
        m=unreal.load_asset(str(ad.package_name))
        if m:
            e=m.get_bounds().box_extent; o=m.get_bounds().origin
            print("  %-46s h=%.0f w=%.0f baseZ=%.0f"%(ad.asset_name, e.z*2, max(e.x,e.y)*2, o.z-e.z))
