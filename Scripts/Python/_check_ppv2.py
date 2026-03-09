"""Check PostProcessVolume settings for GI/reflection overrides."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == 'PostProcessVolume':
        label = a.get_actor_label()
        print(f"PPV: {label}")

        # PostProcessVolume has settings directly
        settings = a.get_editor_property('settings')

        # Check all override flags we care about
        props = [
            'override_dynamic_global_illumination_method',
            'override_reflection_method',
            'override_lumen_scene_lighting_quality',
            'override_lumen_scene_detail',
            'override_lumen_scene_view_distance',
            'override_lumen_final_gather_quality',
            'override_lumen_reflections_quality',
        ]

        for p in props:
            try:
                val = settings.get_editor_property(p)
                print(f"  {p}: {val}")
            except:
                pass

        # Check actual values
        val_props = [
            'dynamic_global_illumination_method',
            'reflection_method',
            'lumen_scene_lighting_quality',
            'lumen_scene_detail',
            'lumen_scene_view_distance',
            'lumen_final_gather_quality',
            'lumen_reflections_quality',
        ]

        for p in val_props:
            try:
                val = settings.get_editor_property(p)
                print(f"  {p}: {val}")
            except:
                pass

        # Is it unbound (infinite extent)?
        try:
            unbound = a.get_editor_property('unbound')
            print(f"  Unbound (infinite): {unbound}")
        except:
            pass

        try:
            priority = a.get_editor_property('priority')
            print(f"  Priority: {priority}")
        except:
            pass
