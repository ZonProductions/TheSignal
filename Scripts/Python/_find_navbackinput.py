import unreal

# Find which widget contains "NavBackInput" and dump its class + key properties.
candidates = [
    "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI",
    "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SaveFileActionBanner",
    "/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SaveFileCard",
]

def dump_tree(bp, path):
    wt = bp.get_editor_property("widget_tree")
    if not wt:
        print("  no widget_tree"); return
    widgets = wt.get_editor_property("all_widgets") if False else None
    # iterate via get_all_widgets
    allw = []
    try:
        allw = list(wt.all_widgets)
    except Exception:
        try:
            allw = unreal.WidgetBlueprintLibrary.get_all_widgets(wt) if False else []
        except Exception:
            pass
    if not allw:
        # fallback: use get_widget_from_name on common names
        return
    for w in allw:
        nm = w.get_name()
        cls = type(w).__name__
        if "NavBack" in nm or "Back" in nm or "Input" in cls or "BoundAction" in cls:
            print("   [%s] %s (%s)" % (path.split('/')[-1], nm, cls))
            for prop in ["input_action","input_action_data","input_actions","trigger_input_action",
                         "bound_action","action","text","input_type_override","display_name"]:
                try:
                    v = w.get_editor_property(prop)
                    if v not in (None, ""):
                        print("        %s = %s" % (prop, v.get_name() if isinstance(v, unreal.Object) and v else v))
                except Exception:
                    pass

for p in candidates:
    bp = unreal.load_asset(p)
    if not bp:
        print("MISSING", p); continue
    print("=== %s ===" % p.split('/')[-1])
    try:
        dump_tree(bp, p)
    except Exception as e:
        print("  err:", e)
