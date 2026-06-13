import unreal
EAL = unreal.EditorAssetLibrary

# Find icon textures wherever they landed
ar = unreal.AssetRegistryHelpers.get_asset_registry()
hits = []
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Texture2D"), True):
    n = str(ad.asset_name)
    if any(k in n for k in ("Pipe", "Shotgun", "Explosive", "AssualtRifle", "Assault",
                            "Pistol", "Rifle", "Grenade", "Viper")):
        hits.append(str(ad.package_name))
print("Candidate icon textures:")
for p in sorted(set(hits)):
    t = unreal.load_asset(p)
    if not isinstance(t, unreal.Texture2D):
        continue
    cs = t.get_editor_property("compression_settings")
    srgb = t.get_editor_property("srgb")
    noa = t.get_editor_property("compression_no_alpha")
    try:
        w = t.blueprint_get_size_x(); h = t.blueprint_get_size_y()
    except Exception:
        w = h = "?"
    print("  %-55s %sx%s | comp=%s srgb=%s noAlpha=%s" %
          (p, w, h, cs, srgb, noa))
