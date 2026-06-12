"""
Fit the grenade into the Kubold melee idle's RIGHT fist — offline, no PIE.

Dev design (session 63): grenade hold = one half of the pipe animation. The
right fist of A_MeleePipe_Idle already grips a cylinder; place the grenade's
long axis along the same channel line the (approved) pipe occupies, centered
on the right fist's finger-ring centroid. Includes the same 7.5° POV trim
pivoting on the left fist that the pipe fit uses.

Prints ThrowableGripOffset/Rotation for ZP_KinemationComponent.h — these are
now relative to the VIEW MESH hand_r (Kubold), not PlayerMesh.
"""
import unreal

IDLE_PATH = '/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle'
GRENADE_SM = '/Game/InventorySystemPro/ExampleContent/Common/Art/Weapons/Grenade/Mesh/SM_grenade'
POV_TRIM_DEG = 7.5   # same trim as the approved pipe fit
GRIP_SCALE = 0.6     # dev-tuned held-grenade scale — pivot correction uses it
SAMPLES = 8

idle = unreal.load_asset(IDLE_PATH)
sm = unreal.load_asset(GRENADE_SM)
assert idle and sm, 'idle anim or grenade mesh missing'

try:
    length = idle.get_play_length()
except Exception:
    length = float(idle.get_editor_property('sequence_length'))

ext = sm.get_bounds().box_extent
borig = sm.get_bounds().origin
axes = {'x': ext.x, 'y': ext.y, 'z': ext.z}
long_axis = max(axes, key=axes.get)
unreal.log(f'grenade long axis: {long_axis}, extents ({ext.x:.1f}, {ext.y:.1f}, {ext.z:.1f})')

opts = unreal.AnimPoseEvaluationOptions()
space = unreal.AnimPoseSpaces.WORLD
GRIP_BONES = ([f'{f}_{j:02d}' for f in ('index', 'middle', 'ring', 'pinky') for j in (1, 2, 3)]
              + ['thumb_02', 'thumb_03'])


def channel(pose, side):
    sx = sy = sz = 0.0
    n = 0
    for f in GRIP_BONES:
        try:
            b = unreal.AnimPoseExtensions.get_bone_pose(pose, f'{f}_{side}', space).translation
            sx += b.x
            sy += b.y
            sz += b.z
            n += 1
        except Exception:
            pass
    assert n >= 8, f'only {n} grip bones for hand_{side}'
    return unreal.Vector(sx / n, sy / n, sz / n)


rel_locs = []
rel_quats = []
for i in range(SAMPLES):
    t = length * i / SAMPLES
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(idle, t, opts)
    cR = channel(pose, 'r')
    cL = channel(pose, 'l')
    hr = unreal.AnimPoseExtensions.get_bone_pose(pose, 'hand_r', space)

    # Same POV trim as the pipe: swing the right-fist point about the left
    # fist, around the camera-forward axis (+Y in this component space).
    arm = unreal.Vector(cR.x - cL.x, cR.y - cL.y, cR.z - cL.z)
    arm = unreal.MathLibrary.rotate_angle_axis(arm, -POV_TRIM_DEG, unreal.Vector(0.0, 1.0, 0.0))
    cR = unreal.Vector(cL.x + arm.x, cL.y + arm.y, cL.z + arm.z)
    dn = arm.normal()

    # Grenade long axis along the fist channel, geometric center AT the
    # right-fist channel center (pivot-corrected, at held scale).
    if long_axis == 'x':
        rotw = unreal.MathLibrary.make_rot_from_x(dn)
    elif long_axis == 'y':
        rotw = unreal.MathLibrary.make_rot_from_y(dn)
    else:
        rotw = unreal.MathLibrary.make_rot_from_z(dn)
    tf = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=rotw, scale=unreal.Vector(1, 1, 1))
    scaled_orig = unreal.Vector(borig.x * GRIP_SCALE, borig.y * GRIP_SCALE, borig.z * GRIP_SCALE)
    ro = tf.transform_direction(scaled_orig)
    pivot = unreal.Vector(cR.x - ro.x, cR.y - ro.y, cR.z - ro.z)
    desired = unreal.Transform(location=pivot, rotation=rotw, scale=unreal.Vector(1, 1, 1))
    rel = unreal.MathLibrary.make_relative_transform(desired, hr)
    rel_locs.append(rel.translation)
    rel_quats.append(rel.rotation)

ax = sum(v.x for v in rel_locs) / SAMPLES
ay = sum(v.y for v in rel_locs) / SAMPLES
az = sum(v.z for v in rel_locs) / SAMPLES
q0 = rel_quats[0]
qx = qy = qz = qw = 0.0
for q in rel_quats:
    s = 1.0 if (q.x * q0.x + q.y * q0.y + q.z * q0.z + q.w * q0.w) >= 0 else -1.0
    qx += s * q.x
    qy += s * q.y
    qz += s * q.z
    qw += s * q.w
norm = (qx * qx + qy * qy + qz * qz + qw * qw) ** 0.5
mean_rot = unreal.Quat(qx / norm, qy / norm, qz / norm, qw / norm).rotator()

unreal.log('=== KUBOLD-FIST GRENADE GRIP (bake into ZP_KinemationComponent.h) ===')
unreal.log(f'ThrowableGripOffset   = FVector({ax:.2f}f, {ay:.2f}f, {az:.2f}f)')
unreal.log(f'ThrowableGripRotation = FRotator({mean_rot.pitch:.2f}f, {mean_rot.yaw:.2f}f, {mean_rot.roll:.2f}f)')
