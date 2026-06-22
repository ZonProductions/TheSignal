import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

glassmats = {}      # any see-through / glass / window material -> count
window_meshes = {}  # mesh names hinting window/glass
backdrop = {}       # actors from BackgroundBuildings / sky / backdrop
classes = {}
for a in eas.get_all_level_actors():
    cls = a.get_class().get_name()
    classes[cls] = classes.get(cls, 0) + 1
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    for c in cs:
        sm = c.get_editor_property("static_mesh")
        if sm:
            p = sm.get_path_name()
            mn = sm.get_name()
            if any(k in mn.lower() for k in ["window","glass","sky","backdrop","building","city"]):
                window_meshes[mn] = window_meshes.get(mn, 0) + 1
            if "BackgroundBuildings" in p or "Backdrop" in p:
                backdrop[mn] = backdrop.get(mn, 0) + 1
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m:
                nm = m.get_name()
                if any(k in nm.lower() for k in ["glass","window","sky","fakeinterior","fakewindow"]):
                    glassmats[nm] = glassmats.get(nm, 0) + 1

print("=== window/glass/sky/backdrop MESHES present ===", window_meshes or "NONE")
print("=== glass/window/sky/fake MATERIALS in use ===", glassmats or "NONE")
print("=== BackgroundBuildings/Backdrop mesh actors ===", backdrop or "NONE")
print("\n=== sky / light / atmosphere actors ===")
for cls,n in sorted(classes.items()):
    if any(k in cls.lower() for k in ["sky","light","atmosphere","fog","backdrop","postprocess","cubemap","reflection"]):
        print("  %-34s x%d" % (cls, n))
