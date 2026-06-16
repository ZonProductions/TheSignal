import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
s.set_editor_property("occlusion_volume_attenuation", 0.2)
s.set_editor_property("occlusion_low_pass_filter_frequency", 450.0)
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("OCC occVol=%s lpf=%s" % (s.get_editor_property("occlusion_volume_attenuation"), s.get_editor_property("occlusion_low_pass_filter_frequency")))
