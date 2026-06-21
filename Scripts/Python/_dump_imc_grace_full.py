import unreal

imc = unreal.load_asset("/Game/Core/Input/IMC_Grace")
dkm = imc.get_editor_property("default_key_mappings")
ms = dkm.get_editor_property("mappings")
print("IMC_Grace mappings (%d):" % len(ms))
for m in ms:
    action = m.get_editor_property("action")
    key = m.get_editor_property("key")
    try:
        kn = str(key.get_editor_property("key_name"))
    except Exception:
        kn = str(key)
    print("   %-26s <- %s" % (action.get_name() if action else "None", kn))

# Does a flashlight Input Action asset exist anywhere?
areg = unreal.AssetRegistryHelpers.get_asset_registry()
ias = areg.get_assets_by_class(unreal.TopLevelAssetPath("/Script/EnhancedInput", "InputAction"), True)
print("\nInputAction assets containing 'Flash':")
found = False
for ad in ias:
    n = str(ad.get_editor_property("asset_name"))
    if "Flash" in n or "flash" in n:
        print("   ", str(ad.get_editor_property("package_name")))
        found = True
if not found:
    print("   (none)")
