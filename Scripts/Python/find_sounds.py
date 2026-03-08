import unreal
registry = unreal.AssetRegistryHelpers.get_asset_registry()
# Search for SoundWave assets
assets = registry.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', 'SoundWave'))
print(f"Found {len(assets)} SoundWave assets")
for a in assets[:20]:
    print(f"  {a.get_full_name()}")
