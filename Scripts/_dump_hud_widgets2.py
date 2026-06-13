import unreal
bp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
gc = bp.get_editor_property("generated_class")
print("generated_class:", gc)
wt = None
for getter in ["widget_tree", "WidgetTree"]:
    try:
        wt = gc.get_editor_property(getter)
        if wt:
            print("got widget_tree via", getter); break
    except Exception as e:
        print(getter, "err", str(e)[:50])

if wt:
    widgets = wt.get_editor_property("all_widgets")
    print("TOTAL widgets:", len(widgets))
    for w in widgets:
        cn = w.get_class().get_name()
        info = "  %-30s %s" % (w.get_name(), cn)
        if isinstance(w, unreal.Image):
            try:
                res = w.get_editor_property("brush").get_editor_property("resource_object")
                info += " | tex: %s" % (res.get_name() if res else "(none)")
            except Exception as e:
                info += " | brush err"
            try:
                info += " | vis: %s" % w.get_editor_property("visibility")
            except Exception:
                pass
        print(info)
