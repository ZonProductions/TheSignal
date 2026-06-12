"""Compute pipe grip: align SM_Pipe's long axis through both palms (live PIE)."""
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
pawn = unreal.GameplayStatics.get_player_character(world, 0) if world else None
assert pawn, 'NO PIE/PAWN'
comps = pawn.get_components_by_class(unreal.SkeletalMeshComponent)
mv = next(c for c in comps if c.get_name() == 'MeleeViewMesh')
pipe = next(c for c in pawn.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'MeleeViewWeapon')

# Pipe long axis from bounds
sm = pipe.get_editor_property('static_mesh')
bounds = sm.get_bounds()
ext = bounds.box_extent
unreal.log(f'PIPE EXTENT: {ext.x:.1f} {ext.y:.1f} {ext.z:.1f}')
axes = {'x': ext.x, 'y': ext.y, 'z': ext.z}
long_axis = max(axes, key=axes.get)
unreal.log(f'LONG AXIS: {long_axis}')

# World transforms of both hands
ws = unreal.RelativeTransformSpace.RTS_WORLD
hr = mv.get_socket_transform('hand_r', ws)
hl = mv.get_socket_transform('hand_l', ws)
hr_pos, hl_pos = hr.translation, hl.translation
d = unreal.Vector(hl_pos.x - hr_pos.x, hl_pos.y - hr_pos.y, hl_pos.z - hr_pos.z)
dist = d.length()
unreal.log(f'HAND_R: {hr_pos}  HAND_L: {hl_pos}  DIST: {dist:.1f}')
dn = d.normal()

# Desired direction in hand_r local space (pipe is child of hand_r socket)
inv = hr.inverse()
d_local = inv.transform_direction(dn)
rel_rot = unreal.MathLibrary.make_rot_from_x(d_local) if long_axis == 'x' else (
    unreal.MathLibrary.make_rot_from_y(d_local) if long_axis == 'y' else unreal.MathLibrary.make_rot_from_z(d_local))

# Pipe pivot: assume center. Put center at midpoint between hands, nudged so more
# shaft extends past the lead (left) hand — held like a bat, lower grip at right hand.
mid = unreal.Vector((hr_pos.x + hl_pos.x) * 0.5, (hr_pos.y + hl_pos.y) * 0.5, (hr_pos.z + hl_pos.z) * 0.5)
rel_loc = inv.transform_location(mid)

pipe.set_relative_rotation(rel_rot, False, False)
pipe.set_relative_location(rel_loc, False, False)
unreal.log(f'APPLIED rel_rot: {rel_rot}  rel_loc: {rel_loc}')
