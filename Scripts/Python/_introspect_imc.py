import unreal
imc = unreal.load_asset("/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IMC_InventoryCharacter")
dkm = imc.get_editor_property("default_key_mappings")
print("default_key_mappings type:", type(dkm).__name__)
try:
    print("len:", len(dkm))
    for x in dkm:
        print("  entry type:", type(x).__name__, "props:", [p for p in dir(x) if not p.startswith("_")][:20])
        break
except Exception as e:
    print("not iterable:", e)
    print("dir:", [p for p in dir(dkm) if not p.startswith("_")][:40])
