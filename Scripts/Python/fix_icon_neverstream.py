"""5.7 import default left new UI textures with never_stream=False -> they
stream and render as white blocks in UMG. The working pre-5.7 Icon_Pistol has
never_stream=True. Match it on all the new icons (+ enforce TC_EDITOR_ICON)."""
import unreal
for name in ["Icon_Rifle", "Icon_Shotgun", "Icon_Pipe", "Icon_Grenade"]:
    p = "/Game/icons/" + name
    t = unreal.load_asset(p)
    if not isinstance(t, unreal.Texture2D):
        print("MISSING", name); continue
    t.set_editor_property("never_stream", True)
    t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    t.set_editor_property("srgb", True)
    t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    unreal.EditorAssetLibrary.save_asset(p)
    print("FIXED %-13s never_stream=%s comp=%s" % (
        name, t.get_editor_property("never_stream"),
        str(t.get_editor_property("compression_settings")).split(".")[-1]))
