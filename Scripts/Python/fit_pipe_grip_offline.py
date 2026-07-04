"""
Compute the pipe grip transform OFFLINE from A_MeleePipe_Idle — no PIE needed.

Samples the idle pose, reads both hand bones in component space, aligns the
pipe's long axis through both palms, and writes MeleeGripOffset/Rotation onto
the BP_GraceCharacter CDO (KinemationComp) so it applies immediately.
Numbers printed for baking into ZP_KinemationComponent.h defaults.
"""
import unreal

IDLE = unreal.load_asset('/Game/TheSignal/Animations/Melee/A_MeleePipe_Idle')
PIPE_SM = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Art/Pipe/SM_Pipe')
assert IDLE and PIPE_SM, 'assets missing'

# --- Sample the idle pose (component space) ---
opts = unreal.AnimPoseEvaluationOptions()
pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(IDLE, 0.5, opts)
space = unreal.AnimPoseSpaces.WORLD  # component space for an AnimPose
hr = unreal.AnimPoseExtensions.get_bone_pose(pose, 'hand_r', space)
hl = unreal.AnimPoseExtensions.get_bone_pose(pose, 'hand_l', space)
hr_pos, hl_pos = hr.translation, hl.translation
unreal.log(f'hand_r CS: {hr_pos.x:.1f} {hr_pos.y:.1f} {hr_pos.z:.1f}')
unreal.log(f'hand_l CS: {hl_pos.x:.1f} {hl_pos.y:.1f} {hl_pos.z:.1f}')

# --- Pipe long axis ---
ext = PIPE_SM.get_bounds().box_extent
unreal.log(f'pipe extent: {ext.x:.1f} {ext.y:.1f} {ext.z:.1f}')
axes = {'x': ext.x, 'y': ext.y, 'z': ext.z}
long_axis = max(axes, key=axes.get)
unreal.log(f'pipe long axis: {long_axis} (half-length {axes[long_axis]:.1f})')

# --- Align long axis through both palms, in hand_r bone space ---
# Direction: rear grip (hand_l) THROUGH front grip (hand_r) and beyond —
# the shaft's long end extends past the right hand, sword-style.
d = unreal.Vector(hr_pos.x - hl_pos.x, hr_pos.y - hl_pos.y, hr_pos.z - hl_pos.z)
dist = d.length()
dn = d.normal()
inv = hr.inverse()
d_local = inv.transform_direction(dn)
if long_axis == 'x':
    rel_rot = unreal.MathLibrary.make_rot_from_x(d_local)
elif long_axis == 'y':
    rel_rot = unreal.MathLibrary.make_rot_from_y(d_local)
else:
    rel_rot = unreal.MathLibrary.make_rot_from_z(d_local)

# Pipe pivot is its center. Rear stub must cover the left hand (dist behind
# hand_r) plus a few cm past it; the rest of the shaft extends forward.
half_len = axes[long_axis]
rear_stub = dist + 10.0  # pipe behind hand_r: left-hand grip + 10cm pommel
along = half_len - rear_stub
center = unreal.Vector(hr_pos.x + dn.x * along, hr_pos.y + dn.y * along, hr_pos.z + dn.z * along)
rel_loc = inv.transform_location(center)

unreal.log(f'hand distance: {dist:.1f}')
unreal.log(f'=== GRIP VALUES (bake into C++) ===')
unreal.log(f'MeleeGripOffset  = FVector({rel_loc.x:.2f}f, {rel_loc.y:.2f}f, {rel_loc.z:.2f}f)')
unreal.log(f'MeleeGripRotation = FRotator({rel_rot.pitch:.2f}f, {rel_rot.yaw:.2f}f, {rel_rot.roll:.2f}f)')

# --- Apply to CDO now (works without rebuild) ---
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
kin = next(c for c in cdo.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
kin.set_editor_property('AZP_MeleeGripOffset', rel_loc)
kin.set_editor_property('AZP_MeleeGripRotation', rel_rot)
unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
unreal.log('APPLIED to BP_GraceCharacter CDO and saved.')
