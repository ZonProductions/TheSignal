"""Measure live grip error and correct it exactly. Draws debug markers."""
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
assert world, 'NOT IN PIE'
pawn = unreal.GameplayStatics.get_player_character(world, 0)
assert pawn, 'NO PAWN'
mv = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'MeleeViewMesh')
pipe = next(c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'MeleeViewWeapon')

ws = unreal.RelativeTransformSpace.RTS_WORLD
hr = mv.get_socket_transform('hand_r', ws)
hl = mv.get_socket_transform('hand_l', ws)
hr_pos, hl_pos = hr.translation, hl.translation
pipe_tf = pipe.get_world_transform()
unreal.log(f'MEASURED hand_r W: {hr_pos.x:.1f} {hr_pos.y:.1f} {hr_pos.z:.1f}')
unreal.log(f'MEASURED hand_l W: {hl_pos.x:.1f} {hl_pos.y:.1f} {hl_pos.z:.1f}')
unreal.log(f'MEASURED pipe   W: {pipe_tf.translation.x:.1f} {pipe_tf.translation.y:.1f} {pipe_tf.translation.z:.1f}')

# Desired: pipe long axis (local Z) through both fists, rear stub past hand_l
d = unreal.Vector(hr_pos.x - hl_pos.x, hr_pos.y - hl_pos.y, hr_pos.z - hl_pos.z)
dist = d.length()
dn = d.normal()
HALF_LEN = 60.2
rear_stub = dist + 10.0
along = HALF_LEN - rear_stub
center = unreal.Vector(hr_pos.x + dn.x * along, hr_pos.y + dn.y * along, hr_pos.z + dn.z * along)
rot_world = unreal.MathLibrary.make_rot_from_z(dn)
desired_world = unreal.Transform(location=center, rotation=rot_world, scale=unreal.Vector(1, 1, 1))

# Error report (how far off the current pipe is)
err = unreal.Vector(center.x - pipe_tf.translation.x, center.y - pipe_tf.translation.y, center.z - pipe_tf.translation.z)
unreal.log(f'POSITION ERROR: {err.length():.1f} cm  ({err.x:.1f}, {err.y:.1f}, {err.z:.1f})')

# SM_Pipe's pivot is NOT at its geometric center — compensate by the local
# bounds origin so the GEOMETRY center (not the pivot) lands on the hand line.
sm = pipe.get_editor_property('static_mesh')
local_origin = sm.get_bounds().origin
rotated_origin = desired_world.transform_direction(local_origin)
pivot_world = unreal.Vector(center.x - rotated_origin.x, center.y - rotated_origin.y, center.z - rotated_origin.z)
desired_world = unreal.Transform(location=pivot_world, rotation=rot_world, scale=unreal.Vector(1, 1, 1))
unreal.log(f'pipe local bounds origin: ({local_origin.x:.1f}, {local_origin.y:.1f}, {local_origin.z:.1f})')

# Exact relative transform vs the hand_r socket
rel = unreal.MathLibrary.make_relative_transform(desired_world, hr)
pipe.set_relative_location(rel.translation, False, False)
pipe.set_relative_rotation(rel.rotation.rotator(), False, False)
unreal.log(f'=== APPLIED (bake later) ===')
unreal.log(f'GripOffset  = FVector({rel.translation.x:.2f}f, {rel.translation.y:.2f}f, {rel.translation.z:.2f}f)')
r = rel.rotation.rotator()
unreal.log(f'GripRotation = FRotator({r.pitch:.2f}f, {r.yaw:.2f}f, {r.roll:.2f}f)')

# Debug draw: red = hand_r, blue = hand_l, green line = intended shaft
unreal.SystemLibrary.draw_debug_sphere(world, hr_pos, 4.0, 12, unreal.LinearColor(1, 0, 0, 1), 20.0, 0.5)
unreal.SystemLibrary.draw_debug_sphere(world, hl_pos, 4.0, 12, unreal.LinearColor(0, 0.3, 1, 1), 20.0, 0.5)
tip = unreal.Vector(center.x + dn.x * HALF_LEN, center.y + dn.y * HALF_LEN, center.z + dn.z * HALF_LEN)
butt = unreal.Vector(center.x - dn.x * HALF_LEN, center.y - dn.y * HALF_LEN, center.z - dn.z * HALF_LEN)
unreal.SystemLibrary.draw_debug_line(world, butt, tip, unreal.LinearColor(0, 1, 0, 1), 20.0, 1.0)
unreal.log('DEBUG MARKERS drawn (20s): red=right hand, blue=left hand, green=intended shaft')
