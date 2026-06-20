import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
# Find Blueprints whose generated class derives from ZP_GraceCharacter
hits = []
for data in ar.get_assets_by_class("Blueprint", search_sub_classes=False):
    pc = data.get_tag_value("ParentClass") if hasattr(data,"get_tag_value") else None
    name = str(data.asset_name)
    if pc and ("GraceCharacter" in str(pc) or "ZP_Grace" in str(pc)):
        hits.append((name, str(data.package_name), str(pc)))
for h in hits:
    print("PLAYER BP:", h)
if not hits:
    # Fallback: scan for BP_ with Grace in name
    for data in ar.get_assets_by_path("/Game", recursive=True):
        n = str(data.asset_name)
        if "Grace" in n and str(data.asset_class_path.asset_name) == "Blueprint":
            print("CANDIDATE:", n, str(data.package_name))
