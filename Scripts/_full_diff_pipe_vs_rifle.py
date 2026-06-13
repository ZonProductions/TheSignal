import unreal
PROPS = [
 "compression_settings","compression_no_alpha","compression_force_alpha_channel",
 "compression_quality","srgb","never_stream","mip_gen_settings","lod_group","filter",
 "address_x","address_y","virtual_texture_streaming","power_of_two_mode","padding_color",
 "lossy_compression_amount","lod_bias","max_texture_size","num_cinematic_mip_levels",
 "deferred_pass_for_volume_textures","downscale","adjust_brightness",
 "adjust_brightness_curve","adjust_saturation","adjust_vibrance","adjust_rgb_curve",
 "adjust_hue","adjust_min_alpha","adjust_max_alpha","chroma_key_texture",
 "chroma_key_color","chroma_key_threshold","flip_green_channel","mip_load_options",
 "use_legacy_gamma","do_scale_mips_for_alpha_coverage","alpha_coverage_thresholds",
 "oodle_texture_sdk_version",
]
pipe = unreal.load_asset("/Game/icons/Icon_Pipe")
rifle = unreal.load_asset("/Game/icons/Icon_Rifle")

def getp(t, p):
    try:
        return str(t.get_editor_property(p))
    except Exception:
        return "<n/a>"

print("=== DIFFS: Icon_Pipe (works) vs Icon_Rifle (broken) ===")
any_diff = False
for p in PROPS:
    a, b = getp(pipe, p), getp(rifle, p)
    if a != b:
        any_diff = True
        print("  %-38s PIPE=%s | RIFLE=%s" % (p, a, b))
if not any_diff:
    print("  NO property differences found — issue is NOT a texture property.")
