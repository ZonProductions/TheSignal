"""Light path pass 2: moon direction via get_forward_vector, skylight hunt
(component-level, any actor), UDS skylight-related properties."""
import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for a in actor_sub.get_all_level_actors():
    if 'Ultra_Dynamic_Sky' in a.get_class().get_name():
        moon = a.get_editor_property('Moon')
        fwd = moon.get_forward_vector()
        unreal.log(f'MOON fwd=({fwd.x:.2f},{fwd.y:.2f},{fwd.z:.2f}) '
                   f'intensity={moon.get_editor_property("intensity")} '
                   f'cast_shadows={moon.get_editor_property("cast_shadows")}')
        # UDS skylight knobs (BP props have spaces)
        for p in ('Sky Light Mode', 'Sky Light Intensity', 'Controlled Sky Light',
                  'Sky Light', 'Night Brightness'):
            try:
                unreal.log(f'UDS "{p}" = {a.get_editor_property(p)}')
            except Exception:
                unreal.log(f'UDS "{p}" — not a property')

# any skylight component anywhere
found = 0
for a in actor_sub.get_all_level_actors():
    comps = a.get_components_by_class(unreal.SkyLightComponent)
    for c in comps:
        found += 1
        unreal.log(f'SKYLIGHT COMP on {a.get_actor_label()} [{a.get_class().get_name()}] '
                   f'intensity={c.get_editor_property("intensity")} '
                   f'visible={c.get_editor_property("visible")} '
                   f'mobility={c.get_editor_property("mobility")}')
unreal.log(f'skylight components found: {found}')

# hidden-in-game state of old night actors
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() in ('NightSkyAtmosphere', 'NightFog'):
        unreal.log(f'{a.get_actor_label()} hidden_in_game={a.get_editor_property("hidden")} '
                   f'hiddenEd={a.is_temporarily_hidden_in_editor()}')
