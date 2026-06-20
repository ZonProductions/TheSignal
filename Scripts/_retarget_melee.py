import unreal
clips = [
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_R",
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Attack_L",
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle",
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Equip",
 "/Game/TheSignal/Animations/Melee/A_MeleePipe_Unequip",
 "/Game/FPPMeleeAnimset/Animations/SwordnShield/FPP_sns_Dodge",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockLoop",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockWalk",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockStart",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockStop",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockImpact1",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockImpact2",
 "/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_BlockImpact3",
]
assets = [unreal.EditorAssetLibrary.find_asset_data(c) for c in clips]
src = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono")
tgt = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male")
rtg = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Animation/MH_Retargeter")
new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(assets, src, tgt, rtg,
        prefix="Marcus_", target_path="/Game/Marcus/MeleeAnims", overwrite_existing_files=True)
print("RESULTS:", len(new))
unreal.EditorAssetLibrary.save_directory("/Game/Marcus/MeleeAnims", False, True)
# verify two animate (arm bone rotation delta)
for nm in ["Marcus_A_MeleePipe_Attack_R","Marcus_FPP_Longs_BlockLoop"]:
    a = unreal.load_asset("/Game/Marcus/MeleeAnims/"+nm)
    if not a: print(nm,"MISSING"); continue
    L=a.get_play_length()
    def rot(b,t):
        r=unreal.AnimationLibrary.get_bone_pose_for_time(a,b,t,False).rotation.rotator(); return (r.pitch,r.yaw,r.roll)
    d=0
    for b in ["upperarm_l","lowerarm_l","hand_r"]:
        x=rot(b,0); y=rot(b,L*0.5); d+=abs(x[0]-y[0])+abs(x[1]-y[1])+abs(x[2]-y[2])
    print(nm,"len",round(L,2),"armRotDelta",round(d,1),"ANIMATED" if d>3 else "STATIC")
