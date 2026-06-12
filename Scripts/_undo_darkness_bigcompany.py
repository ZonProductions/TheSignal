import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
assert 'BigCompany' in w.get_name(), f'wrong map loaded: {w.get_name()}'

for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.PostProcessVolume):
    if a.get_actor_label() != 'PPV_GlobalDarkness':
        continue
    s = a.get_editor_property('settings')
    # UNDO the darkness: stop overriding indirect lighting entirely.
    # KEEP: dynamic_global_illumination_method=NONE (the swimming-pool fix).
    s.set_editor_property('override_indirect_lighting_intensity', False)
    a.set_editor_property('settings', s)
    a.modify()
    unreal.log(f'PPV_GlobalDarkness: indirect_lighting_intensity override REMOVED (darkness undone)')
    unreal.log(f'  kept: GI method override = {s.get_editor_property("override_dynamic_global_illumination_method")} -> {s.get_editor_property("dynamic_global_illumination_method")}')
    break
else:
    unreal.log_warning('PPV_GlobalDarkness not found')
