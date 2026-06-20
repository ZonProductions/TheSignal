import unreal
walk = unreal.load_asset("/Game/Marcus/Anims/Marcus_M_Neutral_Walk_Loop_F")
L = walk.get_play_length()
def samp(bone, t):
    tr = unreal.AnimationLibrary.get_bone_pose_for_time(walk, bone, t, False)
    r = tr.rotation.rotator()
    return (round(r.pitch,1), round(r.yaw,1), round(r.roll,1))
for bone in ["thigh_l","calf_l","thigh_r","spine_01"]:
    a = samp(bone, 0.0); b = samp(bone, L*0.25); c = samp(bone, L*0.5)
    delta = abs(a[0]-b[0])+abs(a[1]-b[1])+abs(a[2]-b[2]) + abs(b[0]-c[0])+abs(b[1]-c[1])+abs(b[2]-c[2])
    print(f"{bone}: t0={a} t.25={b} t.5={c}  totalRotDelta={round(delta,1)} {'ANIMATED' if delta>2 else 'STATIC'}")
