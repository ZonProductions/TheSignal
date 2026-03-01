import unreal

registry = unreal.AssetRegistryHelpers.get_asset_registry()
# Search by path
results = registry.get_assets_by_path("/Game/KINEMATION", recursive=True)
for r in results:
    name = str(r.asset_name)
    if 'tactical' in name.lower() or 'ABP_' in name:
        print(f"{r.package_name}/{r.asset_name}")
