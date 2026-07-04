"""
Fit the pipe STRAIGHT through both fists' grip channels — offline, no PIE.

The Kubold longsword idle holds the weapon straight up through both stacked
fists. This samples A_MeleePipe_Idle at several times across the loop,
computes each fist's grip-channel center (inside the curled fingers, same
CURL model as Scripts/_fit_grip_channel.py — the "almost had it" fit), and
aligns the pipe's long axis dead along the line through both channels.

Averaging over the loop removes the single-frame sway error a live PIE
capture bakes in. Applies to BP_GraceCharacter CDO (KinemationComp) and
saves; prints values for baking into ZP_KinemationComponent.h defaults.
"""
import unreal

IDLE_PATH = '/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle'
PIPE_PATH = '/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe'
POMMEL = 12.0    # cm of pipe butt past the rear (left) fist
SAMPLES = 8
# Dev-tuned POV trim (session 63): swing the shaft this many degrees to the
# CAMERA'S RIGHT, pivoting on the LEFT fist (its grip is perfect — only the
# upper/right-hand end was sitting slightly out of the grip). MeleeViewMesh is
# a camera child at relative yaw=-90, so in this component space the camera's
# forward = +Y and the camera's right = -X; a NEGATIVE rotation about +Y
# swings the upper shaft end toward camera-right.
POV_TRIM_DEG = 7.5

idle = unreal.load_asset(IDLE_PATH)
pipe_sm = unreal.load_asset(PIPE_PATH)
assert idle and pipe_sm, 'idle anim or pipe mesh missing'

try:
    length = idle.get_play_length()
except Exception:
    length = float(idle.get_editor_property('sequence_length'))
unreal.log(f'idle length: {length:.2f}s, sampling {SAMPLES} frames')

# --- Pipe long axis from bounds ---
ext = pipe_sm.get_bounds().box_extent
borig = pipe_sm.get_bounds().origin
axes = {'x': ext.x, 'y': ext.y, 'z': ext.z}
long_axis = max(axes, key=axes.get)
half_len = axes[long_axis]
unreal.log(f'pipe long axis: {long_axis} (half-length {half_len:.1f}), bounds origin ({borig.x:.1f}, {borig.y:.1f}, {borig.z:.1f})')

opts = unreal.AnimPoseEvaluationOptions()
space = unreal.AnimPoseSpaces.WORLD  # component space for an AnimPose

# DO NOT estimate the channel from the wrist→knuckle line: the metacarpals run
# right under the BACK of the hand, so that line lays the shaft against the
# back of the fist (session 63 lesson — dev saw exactly that). In a clenched
# grip the finger joints wrap a ring AROUND the shaft — knuckles on top, mid
# phalanges across the far side, tips curling back, thumb closing the ring on
# the palm side — so their centroid IS the shaft center.
GRIP_BONES = ([f'{f}_{j:02d}' for f in ('index', 'middle', 'ring', 'pinky') for j in (1, 2, 3)]
              + ['thumb_02', 'thumb_03'])


def channel(pose, side):
    """Grip-channel center: centroid of the finger-joint ring around the shaft."""
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
    assert n >= 8, f'only {n} grip bones found for hand_{side}'
    return unreal.Vector(sx / n, sy / n, sz / n)


rel_locs = []
rel_quats = []
dists = []
for i in range(SAMPLES):
    t = length * i / SAMPLES
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(idle, t, opts)
    cR = channel(pose, 'r')
    cL = channel(pose, 'l')
    hr = unreal.AnimPoseExtensions.get_bone_pose(pose, 'hand_r', space)

    # POV trim: rotate the right-fist point around the left fist (the pivot)
    # about the camera-forward axis — pure on-screen swing, left grip untouched.
    arm = unreal.Vector(cR.x - cL.x, cR.y - cL.y, cR.z - cL.z)
    arm = unreal.MathLibrary.rotate_angle_axis(arm, -POV_TRIM_DEG, unreal.Vector(0.0, 1.0, 0.0))
    cR = unreal.Vector(cL.x + arm.x, cL.y + arm.y, cL.z + arm.z)
    if i == 0:
        unreal.log(f'probe cR mesh-space: ({cR.x:.1f}, {cR.y:.1f}, {cR.z:.1f})  cL: ({cL.x:.1f}, {cL.y:.1f}, {cL.z:.1f})')

    d = unreal.Vector(cR.x - cL.x, cR.y - cL.y, cR.z - cL.z)
    dist = d.length()
    dists.append(dist)
    dn = d.normal()

    # Shaft along the fist line: butt clears the rear (left) fist by POMMEL,
    # the long end extends past the right fist (sword convention).
    along = half_len - (dist + POMMEL)
    center = unreal.Vector(cR.x + dn.x * along, cR.y + dn.y * along, cR.z + dn.z * along)
    if long_axis == 'x':
        rotw = unreal.MathLibrary.make_rot_from_x(dn)
    elif long_axis == 'y':
        rotw = unreal.MathLibrary.make_rot_from_y(dn)
    else:
        rotw = unreal.MathLibrary.make_rot_from_z(dn)

    # Pivot is not the geometric center — pull the rotated bounds origin out
    # so the GEOMETRY centerline (not the pivot) lands on the fist line.
    tf = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=rotw, scale=unreal.Vector(1, 1, 1))
    ro = tf.transform_direction(borig)
    pivot = unreal.Vector(center.x - ro.x, center.y - ro.y, center.z - ro.z)
    desired = unreal.Transform(location=pivot, rotation=rotw, scale=unreal.Vector(1, 1, 1))
    rel = unreal.MathLibrary.make_relative_transform(desired, hr)
    rel_locs.append(rel.translation)
    rel_quats.append(rel.rotation)

# --- Average the per-sample fits (hand_r-relative, so rigid-grip frames agree) ---
ax = sum(v.x for v in rel_locs) / SAMPLES
ay = sum(v.y for v in rel_locs) / SAMPLES
az = sum(v.z for v in rel_locs) / SAMPLES
mean_loc = unreal.Vector(ax, ay, az)

q0 = rel_quats[0]
qx = qy = qz = qw = 0.0
for q in rel_quats:
    s = 1.0 if (q.x * q0.x + q.y * q0.y + q.z * q0.z + q.w * q0.w) >= 0 else -1.0
    qx += s * q.x
    qy += s * q.y
    qz += s * q.z
    qw += s * q.w
norm = (qx * qx + qy * qy + qz * qz + qw * qw) ** 0.5
mean_q = unreal.Quat(qx / norm, qy / norm, qz / norm, qw / norm)
mean_rot = mean_q.rotator()

# Spread report — how much the idle sway moves the ideal fit around the mean.
max_loc_err = max(((v.x - ax) ** 2 + (v.y - ay) ** 2 + (v.z - az) ** 2) ** 0.5 for v in rel_locs)
max_ang_err = max(abs(q.angular_distance(mean_q)) * 57.2958 for q in rel_quats)
unreal.log(f'fist distance: {sum(dists)/SAMPLES:.1f}cm (min {min(dists):.1f}, max {max(dists):.1f})')
unreal.log(f'fit spread across loop: max {max_loc_err:.2f}cm, {max_ang_err:.2f} deg from mean')

unreal.log('=== STRAIGHT-LINE GRIP (bake into ZP_KinemationComponent.h) ===')
unreal.log(f'MeleeGripOffset   = FVector({mean_loc.x:.2f}f, {mean_loc.y:.2f}f, {mean_loc.z:.2f}f)')
unreal.log(f'MeleeGripRotation = FRotator({mean_rot.pitch:.2f}f, {mean_rot.yaw:.2f}f, {mean_rot.roll:.2f}f)')

# --- Apply to CDO now so the next PIE uses it without a rebuild ---
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
kin = next(c for c in cdo.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
kin.set_editor_property('AZP_MeleeGripOffset', mean_loc)
kin.set_editor_property('AZP_MeleeGripRotation', mean_rot)
saved = unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
if saved:
    unreal.log('APPLIED to BP_GraceCharacter CDO and SAVED to disk.')
else:
    # UE refuses asset saves while PIE is running — values are live on the
    # in-memory CDO (next PIE spawn uses them) but MUST be saved after PIE ends.
    pie = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world() is not None
    unreal.log_warning(f'APPLIED to in-memory CDO but SAVE FAILED (PIE running: {pie}) — re-save after exiting PIE!')
