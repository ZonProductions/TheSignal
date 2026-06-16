import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
# distance (open-space presence) — occlusion handles room separation
s.set_editor_property("attenuate", True)
s.set_editor_property("spatialize", True)
s.set_editor_property("attenuation_shape_extents", unreal.Vector(350.0, 0.0, 0.0))
s.set_editor_property("falloff_distance", 800.0)
# OCCLUSION — walls block the sound like light
s.set_editor_property("enable_occlusion", True)
s.set_editor_property("occlusion_trace_channel", unreal.CollisionChannel.ECC_VISIBILITY)
s.set_editor_property("occlusion_volume_attenuation", 0.1)        # occluded -> 10% volume
s.set_editor_property("occlusion_low_pass_filter_frequency", 300.0)  # heavily muffled
s.set_editor_property("occlusion_interpolation_time", 0.15)
try:
    s.set_editor_property("use_complex_collision_for_occlusion", False)
except Exception:
    pass
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("OCCLUSION_ON occ=%s chan=%s occVol=%s lpf=%s falloff=%s" % (
    s.get_editor_property("enable_occlusion"),
    s.get_editor_property("occlusion_trace_channel"),
    s.get_editor_property("occlusion_volume_attenuation"),
    s.get_editor_property("occlusion_low_pass_filter_frequency"),
    s.get_editor_property("falloff_distance")))
