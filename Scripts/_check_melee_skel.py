import unreal
clips = [
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle",
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_R",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockLoop",
 "/Game/FPPMeleeAnimset/Animations/SwordnShield/FPP_sns_Dodge",
]
for c in clips:
    a = unreal.load_asset(c)
    sk = a.get_editor_property("skeleton") if a else None
    print(c.split("/")[-1], "->", sk.get_name() if sk else "MISSING")
op = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
print("Operator skeleton:", op.get_editor_property("skeleton").get_name() if op else "?")
