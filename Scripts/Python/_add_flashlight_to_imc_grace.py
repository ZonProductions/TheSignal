import unreal

IMC_PATH = "/Game/Core/Input/IMC_Grace"
IA_PATH  = "/Game/InventorySystemPro/Blueprints/Input/InventoryCharacter/IA_InventoryFlashlight"

imc = unreal.load_asset(IMC_PATH)
ia  = unreal.load_asset(IA_PATH)
assert imc and ia, "IMC or IA missing"

# What keys are already mapped to the flashlight action here (idempotency)?
dkm = imc.get_editor_property("default_key_mappings")
existing = set()
for m in dkm.get_editor_property("mappings"):
    a = m.get_editor_property("action")
    if a == ia:
        existing.add(str(m.get_editor_property("key").get_editor_property("key_name")))
print("Already mapped on IMC_Grace:", existing or "(none)")

for kname in ("F", "Gamepad_RightShoulder"):
    if kname in existing:
        print("  skip (exists):", kname)
        continue
    k = unreal.Key()
    k.set_editor_property("key_name", kname)
    imc.map_key(ia, k)   # adds an FEnhancedActionKeyMapping
    print("  mapped IA_InventoryFlashlight <-", kname)

ok = unreal.EditorAssetLibrary.save_asset(IMC_PATH, only_if_is_dirty=False)
print("SAVED" if ok else "SAVEFAIL", IMC_PATH)

# Verify
dkm = imc.get_editor_property("default_key_mappings")
print("Flashlight mappings now on IMC_Grace:")
for m in dkm.get_editor_property("mappings"):
    if m.get_editor_property("action") == ia:
        print("   <-", m.get_editor_property("key").get_editor_property("key_name"))
