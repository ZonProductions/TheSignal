import unreal

IMC_PATH = "/Game/InventorySystemPro/Blueprints/Input/FirstTimePickup/IMC_InventoryFirstTimePickupNotification"
imc = unreal.load_asset(IMC_PATH)
assert imc, "IMC missing"

# Get the close-notification action ref from existing mappings (path-agnostic).
dkm = imc.get_editor_property("default_key_mappings")
ia = None
for m in dkm.get_editor_property("mappings"):
    a = m.get_editor_property("action")
    if a and "CloseNotification" in a.get_name():
        ia = a; break
assert ia, "CloseNotification action not found"

def key(kn):
    k = unreal.Key(); k.set_editor_property("key_name", kn); return k

# CommonUI shows + triggers the PRIMARY (first) gamepad key. Remove the others so
# Gamepad_FaceButton_Bottom (A = interact/confirm) is the only gamepad face button -> it becomes
# primary: glyph shows A and A dismisses (was Y / FaceButton_Top before).
for kn in ("Gamepad_FaceButton_Top", "Gamepad_FaceButton_Left", "Gamepad_FaceButton_Right"):
    imc.unmap_key(ia, key(kn))
    print("unmapped", kn)

# Make sure A is present.
have = set(str(m.get_editor_property("key").get_editor_property("key_name"))
           for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings")
           if m.get_editor_property("action") == ia)
if "Gamepad_FaceButton_Bottom" not in have:
    imc.map_key(ia, key("Gamepad_FaceButton_Bottom"))
    print("mapped Gamepad_FaceButton_Bottom")

ok = unreal.EditorAssetLibrary.save_asset(IMC_PATH, only_if_is_dirty=False)
print("SAVED" if ok else "SAVEFAIL")

print("Final dismiss mappings:")
for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings"):
    if m.get_editor_property("action") == ia:
        print("   <-", m.get_editor_property("key").get_editor_property("key_name"))
