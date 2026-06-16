import unreal
for n in ["SFX_Crawler_Lurking","SFX_Crawler_Alert","SFX_Crawler_Attack","SFX_Crawler_Attack2"]:
    p = "/Game/Audio/Crawler/%s" % n
    a = unreal.load_asset(p)
    if not a:
        unreal.log_warning("%s = MISSING" % n); continue
    ch = a.get_editor_property("num_channels") if hasattr(a,"get_editor_property") else "?"
    try: ch = a.get_editor_property("num_channels")
    except Exception as e: ch = "err:%s"%e
    looping = "?"
    try: looping = a.get_editor_property("looping")
    except Exception: pass
    unreal.log_warning("%s channels=%s looping=%s class=%s" % (n, ch, looping, a.get_class().get_name()))
