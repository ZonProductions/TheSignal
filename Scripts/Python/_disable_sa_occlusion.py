import unreal
sa = unreal.load_asset("/Game/Audio/SA_EnemyVoice")
s = sa.get_editor_property("attenuation")
s.set_editor_property("enable_occlusion", False)  # muffle is now done in C++ (engine occ mutes these sounds)
sa.set_editor_property("attenuation", s)
unreal.EditorAssetLibrary.save_asset("/Game/Audio/SA_EnemyVoice")
unreal.log_warning("SA occlusion DISABLED (C++ self-occlusion handles muffle). attenuate=%s occ=%s" % (
    s.get_editor_property("attenuate"), s.get_editor_property("enable_occlusion")))
