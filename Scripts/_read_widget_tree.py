import unreal
paths = [
    "/Game/Blueprints/UI/WBP_HUD.WidgetTree",
    "/Game/Blueprints/UI/WBP_HUD.WBP_HUD:WidgetTree",
    "/Game/Blueprints/UI/WBP_HUD.WBP_HUD_C:WidgetTree",
    "/Game/Blueprints/UI/WBP_HUD.Default__WBP_HUD_C:WidgetTree",
]
tree = None
for p in paths:
    try:
        o = unreal.load_object(None, p)
        if o:
            print("GOT tree via:", p, type(o).__name__)
            tree = o; break
    except Exception as e:
        print("no:", p, str(e)[:40])

if tree:
    widgets = tree.get_editor_property("all_widgets")
    print("=== WIDGETS (%d) ===" % len(widgets))
    for w in widgets:
        cn = w.get_class().get_name()
        info = "  %-32s %s" % (w.get_name(), cn)
        if isinstance(w, unreal.Image):
            try:
                res = w.get_editor_property("brush").get_editor_property("resource_object")
                info += " | brush_tex: %s" % (res.get_name() if res else "(NONE)")
            except Exception:
                info += " | brush?"
        print(info)
else:
    print("Could not reach widget tree by any path.")
