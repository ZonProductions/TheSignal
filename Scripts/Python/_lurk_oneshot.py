import unreal
lk = unreal.load_asset("/Game/Audio/Crawler/SFX_Crawler_Lurking")
lk.set_editor_property("looping", False)  # now an intermittent one-shot, not a continuous bed
unreal.EditorAssetLibrary.save_asset("/Game/Audio/Crawler/SFX_Crawler_Lurking")
unreal.log_warning("LURK_LOOP=%s" % lk.get_editor_property("looping"))
