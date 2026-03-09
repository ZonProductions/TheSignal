"""Revert PPV to Lumen, re-enable light shadows, count lights."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# 1. Revert PPV — remove GI/reflection overrides (let project settings handle it)
for a in eas.get_all_level_actors():
    if a.get_class().get_name() == 'PostProcessVolume':
        settings = a.get_editor_property('settings')
        settings.set_editor_property('override_dynamic_global_illumination_method', False)
        settings.set_editor_property('override_reflection_method', False)
        a.set_editor_property('settings', settings)
        print("PPV: Reverted GI and Reflection overrides to project defaults")
        break

# 2. Check how many lights have shadows disabled
no_shadow = 0
has_shadow = 0
total = 0
for a in eas.get_all_level_actors():
    lc = a.get_component_by_class(unreal.LightComponent)
    if not lc:
        continue
    total += 1
    if lc.get_editor_property('cast_shadows'):
        has_shadow += 1
    else:
        no_shadow += 1

print(f"\nLights: {total} total, {has_shadow} with shadows, {no_shadow} without shadows")

# 3. Re-enable shadows on ALL lights
restored = 0
for a in eas.get_all_level_actors():
    lc = a.get_component_by_class(unreal.LightComponent)
    if not lc:
        continue
    if not lc.get_editor_property('cast_shadows'):
        lc.set_editor_property('cast_shadows', True)
        restored += 1

print(f"Re-enabled shadows on {restored} lights")
