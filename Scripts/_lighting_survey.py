import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
unreal.log(f'=== LIGHTING SURVEY: {w.get_name()} ===')

import collections
light_counts = collections.Counter()
lights = []
ppvs = []
misc = []
fixture_counts = collections.Counter()

for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    cls = a.get_class().get_name()
    if 'Light' in cls and 'Flash' not in cls:
        light_counts[cls] += 1
        lights.append(a)
    elif cls == 'PostProcessVolume':
        ppvs.append(a)
    elif cls in ('ExponentialHeightFog', 'SkyAtmosphere', 'VolumetricCloud', 'SphereReflectionCapture', 'BoxReflectionCapture'):
        misc.append(a)
    elif cls == 'StaticMeshActor':
        label = a.get_actor_label().lower()
        for pat in ('floresent', 'fluor', 'lamp', 'light', 'silling', 'ceiling'):
            if pat in label:
                fixture_counts[pat] += 1
                break

unreal.log(f'--- Light actors: {dict(light_counts) if light_counts else "NONE"}')
for L in lights[:15]:
    comps = L.get_components_by_class(unreal.LightComponentBase)
    for c in comps:
        try:
            inten = c.get_editor_property('intensity')
            col = c.get_editor_property('light_color')
            unreal.log(f'  {L.get_actor_label()} ({L.get_class().get_name()}): intensity={inten:.0f} color=({col.r},{col.g},{col.b})')
        except Exception as e:
            unreal.log(f'  {L.get_actor_label()}: {e}')

unreal.log(f'--- PostProcessVolumes: {len(ppvs)}')
for P in ppvs:
    s = P.get_editor_property('settings')
    try:
        unb = P.get_editor_property('unbound')
    except Exception:
        unb = '?'
    try:
        en = P.get_editor_property('enabled')
    except Exception:
        en = '?'
    unreal.log(f'  {P.get_actor_label()}: unbound={unb} priority={P.get_editor_property("priority")} enabled={en}')
    for prop, oprop in (
        ('indirect_lighting_intensity', 'override_indirect_lighting_intensity'),
        ('dynamic_global_illumination_method', 'override_dynamic_global_illumination_method'),
        ('auto_exposure_bias', 'override_auto_exposure_bias'),
        ('auto_exposure_min_brightness', 'override_auto_exposure_min_brightness'),
        ('auto_exposure_max_brightness', 'override_auto_exposure_max_brightness'),
        ('bloom_intensity', 'override_bloom_intensity'),
        ('color_saturation', 'override_color_saturation'),
        ('white_temp', 'override_white_temp'),
        ('vignette_intensity', 'override_vignette_intensity'),
        ('film_grain_intensity', 'override_film_grain_intensity'),
    ):
        try:
            if s.get_editor_property(oprop):
                unreal.log(f'    {prop} = {s.get_editor_property(prop)}')
        except Exception:
            pass

unreal.log(f'--- Atmosphere/fog/reflection actors: {[a.get_actor_label() for a in misc] if misc else "NONE"}')
unreal.log(f'--- Fixture-ish static meshes by label: {dict(fixture_counts) if fixture_counts else "NONE"}')
