import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
s.set_editor_property("attenuate", True)
s.set_editor_property("spatialize", True)
s.set_editor_property("attenuation_shape_extents", unreal.Vector(300.0, 0.0, 0.0))  # full volume within ~3m
s.set_editor_property("falloff_distance", 500.0)                                     # silent by ~8m
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("SA_TIGHTENED inner=%s falloff=%s attenuate=%s" % (
    s.get_editor_property("attenuation_shape_extents"), s.get_editor_property("falloff_distance"), s.get_editor_property("attenuate")))
