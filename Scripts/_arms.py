import unreal
mono = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
print("Operator mono skel:", mono.get_editor_property("skeleton").get_name())
for n in ["SK_Arm_01a","SK_Hand_01a","SK_Glove_01a"]:
    a = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/"+n)
    if not a: print(n,"MISSING"); continue
    sk = a.get_editor_property("skeleton")
    sm = a.get_editor_property("materials")
    slots = [(i, str(s.material_slot_name), s.material_interface.get_name() if s.material_interface else None) for i,s in enumerate(sm)]
    print(n, "| skel:", sk.get_name(), "| mats:", slots)
