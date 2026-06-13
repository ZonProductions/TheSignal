import unreal
gc = unreal.load_object(None, "/Game/Blueprints/UI/WBP_HUD.WBP_HUD_C")
print("generated class:", gc, type(gc).__name__)
wt = gc.get_editor_property("widget_tree")
print("widget_tree:", wt)
widgets = wt.get_editor_property("all_widgets")
print("TOTAL widgets:", len(widgets))
for w in widgets:
    cn = w.get_class().get_name()
    info = "  %-32s %s" % (w.get_name(), cn)
    if isinstance(w, unreal.Image):
        try:
            res = w.get_editor_property("brush").get_editor_property("resource_object")
            info += " | tex: %s" % (res.get_name() if res else "(none)")
        except Exception:
            info += " | brush?"
    print(info)
