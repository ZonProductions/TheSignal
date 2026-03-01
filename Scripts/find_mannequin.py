import unreal

# Try loading known GASP mannequin paths
paths = [
    '/Game/Characters/Mannequins/Meshes/SKM_UEFN_Mannequin',
    '/Game/gasp_Blueprints/Characters/SKM_UEFN_Mannequin',
    '/Game/Characters/SKM_UEFN_Mannequin',
    '/Game/GASP/Characters/Meshes/SKM_UEFN_Mannequin',
]

for p in paths:
    asset = unreal.load_asset(p)
    if asset:
        print(f"FOUND: {p} -> {type(asset).__name__}")
        break
else:
    # Search by substring
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    all_assets = ar.get_all_assets()
    for a in all_assets:
        n = str(a.asset_name)
        if "UEFN" in n or ("Mannequin" in n and "SK" in n):
            print(f"SEARCH: {a.package_name}/{a.asset_name} [{a.asset_class_path}]")
