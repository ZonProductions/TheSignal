import unreal

# Find first-time-pickup related IMCs + any 'pickup/notification/continue/dismiss' input actions.
areg = unreal.AssetRegistryHelpers.get_asset_registry()

imcs = areg.get_assets_by_class(unreal.TopLevelAssetPath("/Script/EnhancedInput", "InputMappingContext"), True)
for ad in imcs:
    name = str(ad.get_editor_property("asset_name"))
    if "FirstTime" in name or "Pickup" in name or "Notification" in name:
        imc = ad.get_asset()
        print("\n=== %s ===" % str(ad.get_editor_property("package_name")))
        try:
            dkm = imc.get_editor_property("default_key_mappings")
            ms = dkm.get_editor_property("mappings")
        except Exception as e:
            print("  err:", e); continue
        for m in ms:
            a = m.get_editor_property("action")
            k = m.get_editor_property("key")
            try:
                kn = str(k.get_editor_property("key_name"))
            except Exception:
                kn = str(k)
            print("   %-30s <- %s" % (a.get_name() if a else "None", kn))
