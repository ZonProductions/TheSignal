import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# Find the inventory character component blueprint
hits = []
for a in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", "Blueprint"), True):
    n = str(a.asset_name)
    if "InventoryCharacterComponent" in n or ("Inventory" in n and "Component" in n):
        hits.append(a)
print("Component BP candidates:", [str(a.get_editor_property("package_name")) for a in hits])

comp_cls = None
for a in hits:
    bp = a.get_asset()
    gen = bp.generated_class() if bp else None
    if gen:
        comp_cls = gen
        print("USING:", str(a.get_editor_property("package_name")))
        break

if comp_cls:
    # List functions of interest (save/load/serialize/items)
    print("=== FUNCTIONS (save/load/item/serialize) ===")
    fns = unreal.get_type_from_class(comp_cls) if False else None
    for f in dir(comp_cls):
        pass
    # Reflection: iterate UFunctions via the class default object's class
    cdo = unreal.get_default_object(comp_cls)
    # Use FindFunction-style probing for likely names
    for name in ["SaveInventory","LoadInventory","Save","Load","SerializeInventory","GetSaveData",
                 "ApplySaveData","SaveToObject","LoadFromObject","AddItemSimple","RemoveItemByDataAsset",
                 "ClearInventory","GetAllItems","GetItems","SaveInventoryData","LoadInventoryData"]:
        f = comp_cls.find_function(name) if hasattr(comp_cls,"find_function") else None
        # fallback
    print("(function probing via python is limited; dumping property ItemSlots struct instead)")

    # Dump ItemSlots inner struct field names via an instance is not possible without PIE;
    # instead introspect the struct used by ItemSlots from the class' property.
    print("=== look for BPI_InventorySaveActor interface + savegame object ===")

# Inventory save assets
for a in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","Blueprint"), True):
    n = str(a.asset_name)
    if "Savegame" in n or "SaveActor" in n or ("Save" in n and "Inventory" in n):
        print("SAVE ASSET:", str(a.get_editor_property("package_name")), "class-ish:", n)
print("DONE")
