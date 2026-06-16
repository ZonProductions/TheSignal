import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
# Occlusion ON, traced on WorldStatic (walls IGNORE Visibility — that was why occlusion did nothing).
s.set_editor_property("enable_occlusion", True)
s.set_editor_property("occlusion_trace_channel", unreal.CollisionChannel.ECC_WORLD_STATIC)
s.set_editor_property("occlusion_volume_attenuation", 0.5)        # half volume through a wall (muffled, NOT muted)
s.set_editor_property("occlusion_low_pass_filter_frequency", 750.0)  # muffled tone, still clearly audible
s.set_editor_property("occlusion_interpolation_time", 0.15)
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("OCCLUSION set: on=%s chan=WorldStatic vol=%s lpf=%s" % (
    s.get_editor_property("enable_occlusion"),
    s.get_editor_property("occlusion_volume_attenuation"),
    s.get_editor_property("occlusion_low_pass_filter_frequency")))
