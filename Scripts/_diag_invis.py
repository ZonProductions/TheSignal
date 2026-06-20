import unreal
inv = unreal.load_asset('/Game/Core/Materials/M_Invisible.M_Invisible')
print("M_Invisible loaded:", inv is not None)
if inv:
    print("  blend_mode:", inv.get_editor_property('blend_mode'))
    print("  dither_opacity_mask:", inv.get_editor_property('dither_opacity_mask'))
    print("  opacity_mask_clip_value:", inv.get_editor_property('opacity_mask_clip_value'))
    print("  two_sided:", inv.get_editor_property('two_sided'))
# Also confirm what the ranged hands material renders as (sanity on MI_Skin_Body_CCMH)
sk = unreal.load_asset('/Game/CharacterCustomizer/Characters/CCMH/Materials/Skin/MI_Skin_Body_CCMH.MI_Skin_Body_CCMH')
if sk:
    base = sk.get_base_material() if hasattr(sk, 'get_base_material') else None
    print("MI_Skin_Body_CCMH base material:", base.get_name() if base else None)
