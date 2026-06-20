import unreal
walk = unreal.load_asset("/Game/Marcus/Anims/Marcus_M_Neutral_Walk_Loop_F")
print("WALK:", walk.get_name() if walk else None)
if walk:
    print("  length:", walk.get_play_length(), "frames:", walk.get_editor_property("number_of_sampled_keys") if False else "?")
    # sample foot_l bone pose at two times via the anim's pose
    try:
        t0 = unreal.AnimationLibrary.get_bone_pose_for_time(walk, "foot_l", 0.0, True)
        t1 = unreal.AnimationLibrary.get_bone_pose_for_time(walk, "foot_l", walk.get_play_length()*0.5, True)
        l0 = t0.translation; l1 = t1.translation
        print("  foot_l @0:", round(l0.x,1), round(l0.y,1), round(l0.z,1))
        print("  foot_l @half:", round(l1.x,1), round(l1.y,1), round(l1.z,1))
        moved = (l1-l0).length()
        print("  foot moved over half-cycle:", round(moved,1), "=> ANIMATED" if moved>1 else "=> STATIC/BAD")
    except Exception as e:
        print("  pose sample err:", e)
