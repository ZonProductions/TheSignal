import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = ar.get_all_assets()

# Find idle, walk fwd, run/jog fwd, sprint fwd for UEFN mannequin
keywords = ["idle", "walk_fwd", "run_fwd", "jog_fwd", "sprint_fwd", "walk_loop", "run_loop", "jog_loop", "sprint_loop"]
for a in all_assets:
    pkg = str(a.package_name)
    name = str(a.asset_name).lower()
    cls = str(a.asset_class_path)
    if "gasp_Characters/UEFN" in pkg and "AnimSequence" in cls:
        if any(k in name for k in keywords):
            print(f"{a.asset_name} -> {pkg}")
