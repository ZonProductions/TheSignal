import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
fog = [a for a in eas.get_all_level_actors() if 'ExponentialHeightFog' in a.get_class().get_name()][0]
fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
for p in sorted(dir(fc)):
    if not p.startswith('_'):
        low = p.lower()
        if any(k in low for k in ['fog','volume','color','density','inscatter','albedo','scatter']):
            print(p)
