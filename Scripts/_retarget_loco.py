import unreal
clips = [
 "/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop",
 "/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F",
 "/Game/gasp_Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F",
 "/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Crouch_Idle_Loop",
 "/Game/gasp_Characters/UEFN_Mannequin/Animations/Crouch_WIP/M_Neutral_Crouch_Loop_F",
]
assets = [unreal.EditorAssetLibrary.find_asset_data(c) for c in clips]
src = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin")
tgt = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male")
rtg = unreal.load_asset("/Game/CharacterCustomizer/Characters/CCMH/Animation/MH_Retargeter")
print("src", src, "tgt", tgt, "rtg", rtg)
new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(assets, src, tgt, rtg,
        prefix="Marcus_", target_path="/Game/Marcus/Anims", overwrite_existing_files=True)
print("RESULTS:", len(new))
for a in new:
    print("  ", a.get_editor_property("object_path") if hasattr(a,'get_editor_property') else a)
