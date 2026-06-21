import unreal

bp = unreal.load_asset("/Game/EasyGameUI/EasySaveGameUI/Core/Widgets/WBP_ESGU_SavesManagerUI")
cdo = unreal.get_default_object(bp.generated_class())
wt = cdo.get_editor_property("WidgetTree")

# Collect all widgets in the tree.
found = None
def visit(w):
    global found
    if not w: return
    if w.get_name() == "NavBackInput" or "NavBack" in w.get_name():
        found = w
# WidgetTree has ForEachWidget via get_all_widgets in py? use root + recursion
root = wt.get_editor_property("root_widget")
stack = [root] if root else []
all_names = []
while stack:
    w = stack.pop()
    if not w: continue
    all_names.append("%s (%s)" % (w.get_name(), type(w).__name__))
    if "NavBack" in w.get_name():
        found = w
    # children
    try:
        n = w.get_num_children() if hasattr(w, "get_num_children") else 0
        for i in range(n):
            stack.append(w.get_child_at(i))
    except Exception:
        pass
    # panel slots fallback
    try:
        for c in w.get_all_children():
            stack.append(c)
    except Exception:
        pass

print("Widgets containing 'NavBack':", [n for n in all_names if "NavBack" in n])
if not found:
    print("NavBackInput NOT found by tree walk. All widget names:")
    for n in all_names: print("   ", n)
else:
    print("\nNavBackInput class:", type(found).__name__)
    print("All editor props with values:")
    for p in dir(found):
        if p.startswith("__"): continue
        try:
            v = found.get_editor_property(p)
        except Exception:
            continue
        if any(k in p.lower() for k in ["input","action","key","use","gamepad","mouse","display","hide","text","brush","info","cond"]):
            print("   %s = %s" % (p, v.get_name() if isinstance(v, unreal.Object) and v else v))
