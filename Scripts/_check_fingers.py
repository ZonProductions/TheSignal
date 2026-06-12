import unreal
opts = unreal.AnimPoseEvaluationOptions()
for path, label in [('/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle', 'RETARGETED'),
                    ('/Game/FPPMeleeAnimset/Animations/Longsword/FPP_Longs_Idle', 'SOURCE')]:
    a = unreal.load_asset(path)
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(a, 0.5, opts)
    sp = unreal.AnimPoseSpaces.WORLD
    try:
        hand = unreal.AnimPoseExtensions.get_bone_pose(pose, 'hand_r', sp).translation
        tip = unreal.AnimPoseExtensions.get_bone_pose(pose, 'middle_03_r', sp).translation
        thumb = unreal.AnimPoseExtensions.get_bone_pose(pose, 'thumb_03_r', sp).translation
        d = ((tip.x - hand.x) ** 2 + (tip.y - hand.y) ** 2 + (tip.z - hand.z) ** 2) ** 0.5
        dt = ((thumb.x - hand.x) ** 2 + (thumb.y - hand.y) ** 2 + (thumb.z - hand.z) ** 2) ** 0.5
        unreal.log(f'{label}: fingertip-to-palm {d:.1f}cm, thumbtip-to-palm {dt:.1f}cm')
    except Exception as e:
        unreal.log_error(f'{label}: {e}')
