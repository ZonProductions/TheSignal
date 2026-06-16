import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
unreal.log_warning("SA attenuate=%s spatialize=%s falloff=%s occ=%s occVol=%s occLPF=%s occChan=%s" % (
    s.get_editor_property("attenuate"), s.get_editor_property("spatialize"),
    s.get_editor_property("falloff_distance"), s.get_editor_property("enable_occlusion"),
    s.get_editor_property("occlusion_volume_attenuation"),
    s.get_editor_property("occlusion_low_pass_filter_frequency"),
    s.get_editor_property("occlusion_trace_channel")))
