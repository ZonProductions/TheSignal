import unreal
EAL = unreal.EditorAssetLibrary

print("=== textures under /Game/icons ===")
for p in EAL.list_assets("/Game/icons", recursive=True):
    a = unreal.load_asset(p.split(".")[0]) if "." in p else unreal.load_asset(p)
    if isinstance(a, unreal.Texture2D):
        cs = a.get_editor_property("compression_settings")
        srgb = a.get_editor_property("srgb")
        noa = a.get_editor_property("compression_no_alpha")
        mip = a.get_editor_property("mip_gen_settings")
        print("  %-45s %sx%s comp=%s srgb=%s noAlpha=%s mip=%s" %
              (a.get_name(), a.blueprint_get_size_x(), a.blueprint_get_size_y(),
               str(cs).split(".")[-1], srgb, noa, str(mip).split(".")[-1]))

print("=== exact DA_ icon names anywhere ===")
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Texture2D"), True):
    n = str(ad.asset_name)
    if n in ("DA_Pipe", "DA_PolicShotgun", "DA_Explosive", "DA_AssualtRifle"):
        print("  ", ad.package_name)
