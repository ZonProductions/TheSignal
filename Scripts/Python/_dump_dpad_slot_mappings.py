import unreal

areg = unreal.AssetRegistryHelpers.get_asset_registry()
assets = areg.get_assets_by_class(unreal.TopLevelAssetPath("/Script/EnhancedInput", "InputMappingContext"), True)

def rows_for(imc):
    out = []
    try:
        dkm = imc.get_editor_property("default_key_mappings")
        ms = dkm.get_editor_property("mappings")
    except Exception:
        return out
    for m in ms:
        action = m.get_editor_property("action")
        key = m.get_editor_property("key")
        try:
            kn = str(key.get_editor_property("key_name"))
        except Exception:
            kn = str(key)
        aname = action.get_name() if action else "None"
        out.append((aname, kn))
    return out

print("Scanning", len(assets), "IMCs...")
for ad in assets:
    imc = ad.get_asset()
    if not imc:
        continue
    path = str(ad.get_editor_property("package_name"))
    rows = rows_for(imc)
    hits = [(a, k) for (a, k) in rows if ("DPad" in k or "Dpad" in k) or ("Slot" in a)]
    if hits:
        print("\n=== %s ===" % path)
        for a, k in hits:
            print("   %-26s <- %s" % (a, k))
