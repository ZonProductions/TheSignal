import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()

mats = {}
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    label = a.get_actor_label().lower()
    if not any(p in label for p in ('floor', 'carpet', 'wood', 'kaf')):
        continue
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m:
                mats.setdefault(m.get_path_name(), set()).add(a.get_actor_label())

unreal.log(f'{len(mats)} floor materials in level:')
for path, actors in mats.items():
    m = unreal.load_asset(path.split('.')[0])
    cls = m.get_class().get_name() if m else '?'
    unreal.log(f'  {path.split(chr(47))[-1]} ({cls}) — used by e.g. {sorted(actors)[0]} (+{len(actors)-1})')
    if isinstance(m, unreal.MaterialInstanceConstant):
        for sp in m.scalar_parameter_values:
            info = sp.get_editor_property('parameter_info')
            unreal.log(f'    scalar: {info.get_editor_property("name")} = {sp.get_editor_property("parameter_value"):.2f}')
