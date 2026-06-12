"""Light path diagnosis: moon direction at ToD=200, skylight, PPV settings."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

for a in actor_sub.get_all_level_actors():
    cls = a.get_class().get_name()
    lbl = a.get_actor_label()
    if 'Ultra_Dynamic_Sky' in cls:
        moon = a.get_editor_property('Moon')
        rot = moon.get_editor_property('component_to_world').rotation.rotator()
        fwd = rot.get_forward_vector()
        unreal.log(f'MOON dir rot={rot} fwd=({fwd.x:.2f},{fwd.y:.2f},{fwd.z:.2f}) '
                   f'intensity={moon.get_editor_property("intensity")} '
                   f'mobility={moon.get_editor_property("mobility")}')
        sun = a.get_editor_property('Sun')
        unreal.log(f'SUN intensity={sun.get_editor_property("intensity")}')
        try:
            slc = a.get_editor_property('Controlled Sky Light')
            unreal.log(f'UDS Controlled Sky Light = {slc}')
        except Exception as ex:
            unreal.log(f'UDS skylight prop: {ex}')
    elif isinstance(a, unreal.SkyLight):
        c = a.get_components_by_class(unreal.SkyLightComponent)[0]
        unreal.log(f'SKYLIGHT {lbl} intensity={c.get_editor_property("intensity")} '
                   f'mobility={c.get_editor_property("mobility")} '
                   f'hidden={a.is_temporarily_hidden_in_editor()}')
    elif isinstance(a, unreal.PostProcessVolume):
        s = a.get_editor_property('settings')
        on = []
        for p in dir(s):
            if p.startswith('override_') and s.get_editor_property(p):
                v = p.replace('override_', '')
                try:
                    on.append(f'{v}={s.get_editor_property(v)}')
                except Exception:
                    on.append(v)
        unreal.log(f'PPV {lbl} unbound={a.get_editor_property("unbound")} '
                   f'priority={a.get_editor_property("priority")} overrides: {on}')
    elif isinstance(a, unreal.DirectionalLight):
        c = a.get_components_by_class(unreal.DirectionalLightComponent)[0]
        unreal.log(f'DIRLIGHT {lbl} intensity={c.get_editor_property("intensity")} '
                   f'hiddenEd={a.is_temporarily_hidden_in_editor()}')
    elif isinstance(a, (unreal.ExponentialHeightFog, unreal.SkyAtmosphere)):
        unreal.log(f'{cls} {lbl} hiddenEd={a.is_temporarily_hidden_in_editor()}')
