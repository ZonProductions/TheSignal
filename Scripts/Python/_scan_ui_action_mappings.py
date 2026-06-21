import unreal

areg = unreal.AssetRegistryHelpers.get_asset_registry()
imcs = areg.get_assets_by_class(unreal.TopLevelAssetPath("/Script/EnhancedInput", "InputMappingContext"), True)

# Show, per IMC, all mappings whose action name looks UI/navigation/select/back/close related.
KEYWORDS = ["Select","Back","Close","Nav","Navigate","Confirm","Accept","Cancel","UI_"]
for ad in imcs:
    imc = ad.get_asset()
    if not imc: continue
    try:
        ms = imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
    except Exception:
        continue
    rows = []
    for m in ms:
        a = m.get_editor_property("action")
        if not a: continue
        an = a.get_name()
        if any(k in an for k in KEYWORDS):
            k = m.get_editor_property("key")
            try: kn = str(k.get_editor_property("key_name"))
            except Exception: kn = str(k)
            rows.append((an, kn))
    if rows:
        print("\n=== %s ===" % str(ad.get_editor_property("asset_name")))
        for an, kn in rows:
            print("   %-30s <- %s" % (an, kn))
