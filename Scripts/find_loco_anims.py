import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = ar.get_all_assets()
for a in all_assets:
    pkg = str(a.package_name)
    name = str(a.asset_name)
    cls = str(a.asset_class_path)
    if "gasp" in pkg.lower() and ("AnimSequence" in cls or "BlendSpace" in cls):
        nl = name.lower()
        if any(k in nl for k in ["walk", "run", "idle", "jog", "sprint", "loco", "blend"]):
            print(f"{pkg}/{name} [{cls}]")
