import unreal

def dump_struct(name):
    for cand in [name, "F"+name, name[1:] if name.startswith("F") else name]:
        cls = getattr(unreal, cand, None)
        if cls is None:
            continue
        try:
            inst = cls()
        except Exception as e:
            print(f"{cand}: cannot instantiate ({e})"); continue
        print(f"=== {cand} fields ===")
        for attr in sorted(dir(inst)):
            if attr.startswith("_") or attr[0].isupper():
                continue
            try:
                v = getattr(inst, attr)
                if callable(v):
                    continue
                print(f"   {attr} = {v!r}  ({type(v).__name__})")
            except Exception:
                pass
        return cand
    print(f"{name}: not exposed to Python")
    return None

dump_struct("FItemSlot")
dump_struct("FItemWithAmount")
dump_struct("FInventorySaveData")

# Inventory component class + save-ish functions
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for a in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","Blueprint"), True):
    n = str(a.asset_name)
    if n == "BP_InventoryCharacterComponent":
        bp = a.get_asset(); gen = bp.generated_class() if bp else None
        print("COMPONENT:", str(a.get_editor_property("package_name")), "class=", gen)
        if gen:
            cdo = unreal.get_default_object(gen)
            for fn in ["SaveInventory","LoadInventory","GatherSaveData","ApplySaveData","GetSaveData",
                       "Save","Load","SerializeInventory","SaveToSlot","LoadFromSlot","ClearInventory",
                       "GetItemSlots","AddItemSimple","RemoveItemByDataAsset","GetAllItemsWithAmount"]:
                # probe by attempting attribute (snake_case) presence
                snake = ''.join(['_'+c.lower() if c.isupper() else c for c in fn]).lstrip('_')
                has = hasattr(cdo, snake) or hasattr(gen, snake)
                if has:
                    print("   has fn-ish:", fn, "->", snake)
        break
print("DONE")
