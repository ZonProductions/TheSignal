import unreal

print("=== M_Invisible ===")
inv = unreal.load_asset('/Game/Core/Materials/M_Invisible.M_Invisible')
if inv:
    print("blend_mode:", inv.get_editor_property('blend_mode'))
    try:
        conn = unreal.MaterialEditingLibrary.get_inputs_for_material_property(inv, unreal.MaterialProperty.MP_OPACITY_MASK)
        print("opacity_mask inputs:", conn)
    except Exception as e:
        print("opacity check err:", e)
else:
    print("M_Invisible NOT FOUND")

print("=== SK_Hand_01a materials ===")
h = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SK_Hand_01a.SK_Hand_01a')
if h:
    for i, m in enumerate(h.materials):
        mi = m.material_interface
        print("  {}: slot='{}' mat='{}'".format(i, m.material_slot_name, mi.get_name() if mi else 'None'))
else:
    print("SK_Hand_01a NOT FOUND")

print("=== MI_Skin_Body_CCMH ===")
sk = unreal.load_asset('/Game/CharacterCustomizer/Characters/CCMH/Materials/Skin/MI_Skin_Body_CCMH.MI_Skin_Body_CCMH')
print("loaded:", sk is not None, sk.get_class().get_name() if sk else "")
if sk:
    parent = sk.get_editor_property('parent') if hasattr(sk, 'get_editor_property') else None
    print("parent:", parent.get_name() if parent else None)
