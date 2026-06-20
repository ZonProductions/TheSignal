import unreal
for n in ["SK_Shirt_01a_FPP","SK_Shirt_01a"]:
    a = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/"+n)
    if not a: print(n,"MISSING"); continue
    sk = a.get_editor_property("skeleton")
    sm = a.get_editor_property("materials")
    slots=[(i,str(s.material_slot_name), s.material_interface.get_name() if s.material_interface else None) for i,s in enumerate(sm)]
    print(n,"| skel:",sk.get_name(),"| mats:",slots)
