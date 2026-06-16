import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
s.set_editor_property("enable_occlusion", False)   # LOS-gating makes occlusion redundant + it was muting everything
s.set_editor_property("attenuate", True)
s.set_editor_property("spatialize", True)
s.set_editor_property("falloff_distance", 1200.0)  # carry across a room in LOS
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("OCC_OFF occ=%s falloff=%s" % (s.get_editor_property("enable_occlusion"), s.get_editor_property("falloff_distance")))
