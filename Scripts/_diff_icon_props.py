import unreal
PROPS = ["virtual_texture_streaming", "never_stream", "srgb",
         "compression_settings", "compression_no_alpha", "mip_gen_settings",
         "lod_group", "filter", "power_of_two_mode", "lossy_compression_amount"]
for name in ["Icon_Pistol", "Icon_Rifle", "Icon_Shotgun", "Icon_Pipe", "Icon_Grenade"]:
    t = unreal.load_asset("/Game/icons/" + name)
    if not isinstance(t, unreal.Texture2D):
        print(name, "MISSING"); continue
    vals = []
    for p in PROPS:
        try:
            v = t.get_editor_property(p)
            vals.append("%s=%s" % (p, str(v).split(".")[-1].rstrip(">")))
        except Exception:
            vals.append("%s=?" % p)
    print("%-13s %s" % (name, " ".join(vals)))
