import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
print("=== textures named Icon_* or matching new weapon icons ===")
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Texture2D"), True):
    n = str(ad.asset_name)
    if n.startswith("Icon_") or n in ("DA_Pipe","DA_PolicShotgun","DA_Explosive",
                                      "Rifle","Shotgun","Pipe","Grenade","Pistol"):
        t = unreal.load_asset(str(ad.package_name))
        cs = str(t.get_editor_property("compression_settings")).split(".")[-1]
        noa = t.get_editor_property("compression_no_alpha")
        print("  %-50s %sx%s comp=%s noAlpha=%s" %
              (ad.package_name, t.blueprint_get_size_x(), t.blueprint_get_size_y(), cs, noa))
