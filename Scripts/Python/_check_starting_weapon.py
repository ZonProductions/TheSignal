import unreal
print("NATIVE CDO StartingWeaponItem:", unreal.get_default_object(unreal.ZP_GraceCharacter).get_editor_property("StartingWeaponItem"))

ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Blueprint"), True)
found = 0
for a in assets:
    pc = str(a.get_tag_value("ParentClass") or "")
    npc = str(a.get_tag_value("NativeParentClass") or "")
    if "GraceCharacter" not in pc and "GraceCharacter" not in npc:
        continue
    found += 1
    bp = a.get_asset()
    gen = bp.generated_class() if bp else None
    val = "n/a"
    if gen:
        try:
            val = unreal.get_default_object(gen).get_editor_property("StartingWeaponItem")
        except Exception as e:
            val = "ERR:%s" % e
    print("BP CHILD:", a.get_editor_property("package_name"), "| ParentClass=", pc or npc, "| StartingWeaponItem=", val)
print("BP children of GraceCharacter found:", found)
print("DONE")
