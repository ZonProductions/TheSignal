import unreal

ar = unreal.AssetRegistryHelpers.get_asset_registry()
base = "/Game/__ExternalActors__/Variant_Horror/Lvl_Horror"
assets = ar.get_assets_by_path(base, recursive=True, include_only_on_disk_assets=False)
print("EXTERNAL_ACTOR_ASSETS:", len(assets))

ppv_obj = None
fog_obj = None
for ad in assets:
    cls = str(ad.asset_class_path.asset_name) if hasattr(ad, "asset_class_path") else str(ad.asset_class)
    if cls in ("PostProcessVolume", "ExponentialHeightFog"):
        obj = ad.get_asset()   # loads the actual external actor object
        print("FOUND", cls, "->", str(ad.package_name), "obj:", type(obj).__name__ if obj else None)
        if cls == "PostProcessVolume":
            ppv_obj = obj
        else:
            fog_obj = obj

print("PPV_OBJ:", ppv_obj, "| FOG_OBJ:", fog_obj)
if ppv_obj and isinstance(ppv_obj, unreal.PostProcessVolume):
    s = ppv_obj.get_editor_property("settings")
    print("PPV readable | exposure_bias:", s.get_editor_property("auto_exposure_bias"),
          "| vignette:", s.get_editor_property("vignette_intensity"))
if fog_obj and isinstance(fog_obj, unreal.ExponentialHeightFog):
    fc = fog_obj.get_editor_property("component")
    print("FOG readable | density:", fc.get_editor_property("fog_density"))
