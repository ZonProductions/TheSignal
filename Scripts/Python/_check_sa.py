import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
unreal.log_warning("SA attenuate=%s spatialize=%s falloff=%s shape=%s ext=%s" % (
    s.get_editor_property("attenuate"),
    s.get_editor_property("spatialize"),
    s.get_editor_property("falloff_distance"),
    s.get_editor_property("attenuation_shape"),
    s.get_editor_property("attenuation_shape_extents")))
lk = unreal.load_asset("/Game/Audio/Crawler/SFX_Crawler_Lurking")
unreal.log_warning("LURK channels=%s looping=%s" % (lk.get_editor_property("num_channels"), lk.get_editor_property("looping")))
