"""Pass 3: map the blackbox enclosure. List ALL actors with dark/black
materials or SM_Cube meshes with ext>400, any Z, near the building.
Group by which face of the building they sit on."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

DARK = ('dark', 'black')
rows = []
for a in actor_sub.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    o, e = a.get_actor_bounds(False)
    if max(e.x, e.y, e.z) < 400:
        continue
    mats, mesh = [], ''
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        sm = c.get_editor_property('static_mesh')
        if sm:
            mesh = sm.get_name()
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            mats.append(m.get_name() if m else 'None')
    if not any(any(d in m.lower() for d in DARK) for m in mats):
        continue
    rows.append((a.get_actor_label(), mesh, o, e, mats))

unreal.log(f'--- {len(rows)} large dark-material actors ---')
for lbl, mesh, o, e, mats in sorted(rows, key=lambda r: (r[2].x, r[2].y)):
    unreal.log(f'{lbl} mesh={mesh} X {o.x-e.x:.0f}..{o.x+e.x:.0f} Y {o.y-e.y:.0f}..{o.y+e.y:.0f} Z {o.z-e.z:.0f}..{o.z+e.z:.0f} mats={set(mats)}')
