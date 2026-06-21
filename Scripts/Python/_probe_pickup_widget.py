import unreal

p = "/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets/WBP_FirstTimePickupNotification_Horror"
wbp = unreal.load_asset(p)
wt = unreal.find_object(wbp, "WidgetTree")
print("wt:", wt)
print("wt methods:", [m for m in dir(wt) if not m.startswith("_")][:40])

# Try to get the root widget by several accessors.
root = None
for acc in ["RootWidget", "root_widget"]:
    try:
        root = wt.get_editor_property(acc)
        print("root via %s: %s" % (acc, root))
        if root: break
    except Exception as e:
        print("  %s failed: %s" % (acc, e))

def dump(w, depth=0):
    if not w: return
    cls = w.get_class().get_name()
    print("  " * depth + "- %s (%s)" % (w.get_name(), cls))
    if "Displayer" in cls or "InputPrompt" in cls:
        for prop in ["UseInputAction?","GamepadKey","MouseKeyboardKey","DisplayConditions","TextToDisplay"]:
            try:
                v = w.get_editor_property(prop)
                if isinstance(v, unreal.Struct):
                    try: v = v.get_editor_property("key_name")
                    except Exception: pass
                print("  " * depth + "     %s = %s" % (prop, v))
            except Exception:
                pass
    # recurse children
    try:
        for i in range(w.get_children_count()):
            dump(w.get_child_at(i), depth+1)
    except Exception:
        try:
            c = w.get_content()
            if c: dump(c, depth+1)
        except Exception:
            pass

if root:
    dump(root)
