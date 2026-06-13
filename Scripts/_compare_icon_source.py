import unreal
for p in ["/Game/icons/Icon_Pistol", "/Game/icons/Icon_Rifle",
          "/Game/icons/Icon_Shotgun", "/Game/icons/Icon_Pipe", "/Game/icons/Icon_Grenade"]:
    t = unreal.load_asset(p)
    if not isinstance(t, unreal.Texture2D):
        print(p, "MISSING"); continue
    src = t.get_editor_property("source")
    try:
        fmt = src.get_editor_property("format")
    except Exception as e:
        fmt = "err:" + str(e)[:30]
    cs = str(t.get_editor_property("compression_settings")).split(".")[-1]
    print("%-28s src_fmt=%s comp=%s srgb=%s noAlpha=%s" % (
        t.get_name(), str(fmt).split(".")[-1], cs,
        t.get_editor_property("srgb"), t.get_editor_property("compression_no_alpha")))
