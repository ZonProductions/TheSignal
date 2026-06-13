import unreal
tree = unreal.load_object(None, "/Game/Blueprints/UI/WBP_HUD.WBP_HUD:WidgetTree")
print("tree:", tree, type(tree).__name__)

# Method? property?
cands = [m for m in dir(tree) if any(k in m.lower() for k in ("widget","child","root"))]
print("candidates:", cands)

widgets = []
# Try common accessors
for attr in ["get_all_widgets", "get_widgets", "all_widgets", "widget_tree"]:
    try:
        v = getattr(tree, attr)
        v = v() if callable(v) else v
        if v:
            print("via", attr, "->", len(v) if hasattr(v,"__len__") else v)
            if hasattr(v, "__iter__"):
                widgets = list(v); break
    except Exception as e:
        print(attr, "err", str(e)[:40])

# Fallback: root + recurse
if not widgets:
    try:
        root = tree.get_editor_property("root_widget")
        print("root:", root.get_name() if root else None)
    except Exception as e:
        print("root err", str(e)[:40])

print("=== IMAGE widgets ===")
for w in widgets:
    if isinstance(w, unreal.Image):
        try:
            res = w.get_editor_property("brush").get_editor_property("resource_object")
            tex = res.get_name() if res else "(NONE)"
        except Exception:
            tex = "?"
        print("  %-30s brush_tex=%s" % (w.get_name(), tex))
    else:
        print("  (%s) %s" % (w.get_class().get_name(), w.get_name()))
