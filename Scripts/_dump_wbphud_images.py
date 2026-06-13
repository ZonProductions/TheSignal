import unreal
bp = unreal.load_asset("/Game/Core/UI/WBP_HUD")
if not bp:
    # try to locate it
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/UMGEditor", "WidgetBlueprint"), True):
        if "WBP_HUD" in str(ad.asset_name):
            print("FOUND WBP_HUD at:", ad.package_name)
            bp = unreal.load_asset(str(ad.package_name))
            break
print("bp:", bp)
if bp:
    wt = bp.get_editor_property("widget_tree") if hasattr(bp, "get_editor_property") else None
    # Use the generated class CDO widget tree
    try:
        tree = bp.widget_tree
    except Exception:
        tree = wt
    widgets = []
    try:
        widgets = tree.get_editor_property("all_widgets") if tree else []
    except Exception as e:
        print("all_widgets err:", e)
    print("Image/widget list:")
    for w in widgets:
        cn = w.get_class().get_name()
        line = "  %s : %s" % (w.get_name(), cn)
        if isinstance(w, unreal.Image):
            try:
                b = w.get_editor_property("brush")
                res = b.get_editor_property("resource_object")
                line += " | texture: %s" % (res.get_name() if res else None)
            except Exception as e:
                line += " | brush err %s" % str(e)[:30]
        print(line)
