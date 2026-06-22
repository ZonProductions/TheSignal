import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

mesh_count = {}        # mesh name -> count
mesh_path = {}         # mesh name -> full path
window_actors = []     # actors whose mesh name has "window"

for a in eas.get_all_level_actors():
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    for c in comps:
        sm = c.get_editor_property("static_mesh")
        if not sm:
            continue
        mn = sm.get_name()
        mesh_count[mn] = mesh_count.get(mn, 0) + 1
        mesh_path[mn] = sm.get_path_name()
        if "window" in mn.lower():
            window_actors.append((a.get_actor_label(), mn))

print("=== WINDOW meshes (mesh name contains 'window') ===")
for mn in sorted([m for m in mesh_count if "window" in m.lower()]):
    print("  %-40s x%-4d  %s" % (mn, mesh_count[mn], mesh_path[mn]))

print("\n=== WALL meshes (mesh name contains 'wall') ===")
for mn in sorted([m for m in mesh_count if "wall" in m.lower()]):
    print("  %-40s x%-4d  %s" % (mn, mesh_count[mn], mesh_path[mn]))

print("\n=== ALL meshes by count (top 40) ===")
for mn, c in sorted(mesh_count.items(), key=lambda kv: -kv[1])[:40]:
    print("  x%-5d %-40s %s" % (c, mn, mesh_path[mn]))
