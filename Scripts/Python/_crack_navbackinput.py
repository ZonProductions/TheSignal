import unreal

base = "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI"
wbp = unreal.load_asset(base)
wt = unreal.find_object(wbp, "WidgetTree")
nav = unreal.find_object(wt, "NavBackInput")
print("NavBackInput:", nav)
if nav:
    print("class:", nav.get_class().get_name())
    # Dump ALL editor-visible properties so I see the REAL names (no guessing).
    print("--- all props ---")
    try:
        for prop in nav.get_editor_property_names() if hasattr(nav, "get_editor_property_names") else []:
            pass
    except Exception:
        pass
    # Try the displayer's known BP var names + snake variants
    for p in ["UseInputAction?", "use_input_action", "InputActionInfos", "input_action_infos",
              "GamepadKey", "gamepad_key", "MouseKeyboardKey", "mouse_keyboard_key",
              "DisplayConditions", "display_conditions", "HideKeyForOtherDevices?",
              "hide_key_for_other_devices", "TextToDisplay", "text_to_display"]:
        try:
            v = nav.get_editor_property(p)
            print("  PROP %-24s = %s" % (p, v.get_name() if isinstance(v, unreal.Object) and v else v))
        except Exception:
            pass
