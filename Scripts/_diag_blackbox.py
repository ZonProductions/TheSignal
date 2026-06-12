"""Find the 'black box' around the building in Building1_3rdFloor.
Lists every visible actor whose bounds ENCLOSE the building footprint
(center -11105,-389, floor3 Z ~960-1480) — i.e. candidates for a giant
light-blocker shell. Also lists window-wall pane materials for reference."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.log(f'=== map: {w.get_name()} ===')

# building footprint (from strip + backdrop scripts)
BX = (-24256.0, 2045.0)
BY = (-2155.0, 1378.0)
BZ = (960.0, 1480.0)

hits = []
for a in actor_sub.get_all_level_actors():
    try:
        o, e = a.get_actor_bounds(False)
    except Exception:
        continue
    if e.x < 1.0 and e.y < 1.0:
        continue
    # bounds enclose the whole footprint?
    if (o.x - e.x <= BX[0] and o.x + e.x >= BX[1] and
        o.y - e.y <= BY[0] and o.y + e.y >= BY[1] and
        o.z - e.z <= BZ[0] and o.z + e.z >= BZ[1]):
        try:
            hig = a.get_editor_property('hidden')
        except Exception:
            hig = '?'
        info = f'{a.get_actor_label()} [{a.get_class().get_name()}] hiddenInGame={hig} hiddenEd={a.is_temporarily_hidden_in_editor()} bounds X {o.x-e.x:.0f}..{o.x+e.x:.0f} Y {o.y-e.y:.0f}..{o.y+e.y:.0f} Z {o.z-e.z:.0f}..{o.z+e.z:.0f}'
        mats = []
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = c.get_editor_property('static_mesh')
            if sm:
                info += f' mesh={sm.get_name()}'
            for i in range(c.get_num_materials()):
                m = c.get_material(i)
                mats.append(m.get_name() if m else 'None')
        if mats:
            info += f' mats={mats}'
        hits.append(info)

unreal.log(f'--- {len(hits)} actors enclosing the building footprint ---')
for h in hits:
    unreal.log(h)

# also: anything with 'box'/'block'/'dark'/'shell'/'sky' in label that is big
unreal.log('--- big label-suspects ---')
for a in actor_sub.get_all_level_actors():
    lbl = a.get_actor_label().lower()
    if any(k in lbl for k in ('box', 'block', 'dark', 'shell', 'cube', 'sky', 'fog', 'night')):
        try:
            o, e = a.get_actor_bounds(False)
        except Exception:
            continue
        if max(e.x, e.y, e.z) > 2000:
            try:
                hig = a.get_editor_property('hidden')
            except Exception:
                hig = '?'
            unreal.log(f'{a.get_actor_label()} [{a.get_class().get_name()}] ext=({e.x:.0f},{e.y:.0f},{e.z:.0f}) @({o.x:.0f},{o.y:.0f},{o.z:.0f}) hiddenInGame={hig} hiddenEd={a.is_temporarily_hidden_in_editor()}')
