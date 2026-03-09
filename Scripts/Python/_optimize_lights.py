"""Optimize lights: disable shadows on all interior lights, reduce attenuation."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

shadows_disabled = 0
total_lights = 0

for a in eas.get_all_level_actors():
    lc = a.get_component_by_class(unreal.LightComponent)
    if not lc:
        continue
    total_lights += 1

    # Disable shadow casting on all lights
    if lc.get_editor_property('cast_shadows'):
        lc.set_editor_property('cast_shadows', False)
        shadows_disabled += 1

print(f"Total lights: {total_lights}")
print(f"Shadows disabled: {shadows_disabled}")
