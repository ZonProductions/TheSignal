import unreal

base = "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI"
wbp = unreal.load_asset(base)
wt = unreal.find_object(wbp, "WidgetTree")
nav = unreal.find_object(wt, "NavBackInput")
assert nav, "NavBackInput not found"
print("before: UseInputAction?=%s Gamepad=%s KBM=%s" % (
    nav.get_editor_property("UseInputAction?"),
    nav.get_editor_property("GamepadKey").get_editor_property("key_name"),
    nav.get_editor_property("MouseKeyboardKey").get_editor_property("key_name")))

def key(kn):
    k = unreal.Key()
    k.set_editor_property("key_name", kn)
    return k

nav.set_editor_property("UseInputAction?", False)
nav.set_editor_property("GamepadKey", key("Gamepad_FaceButton_Right"))
nav.set_editor_property("MouseKeyboardKey", key("Escape"))

print("after:  UseInputAction?=%s Gamepad=%s KBM=%s" % (
    nav.get_editor_property("UseInputAction?"),
    nav.get_editor_property("GamepadKey").get_editor_property("key_name"),
    nav.get_editor_property("MouseKeyboardKey").get_editor_property("key_name")))

unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
ok = unreal.EditorAssetLibrary.save_asset(base, only_if_is_dirty=False)
print("compiled + saved:", ok)
