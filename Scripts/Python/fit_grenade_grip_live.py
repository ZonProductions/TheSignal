"""
Fit the held grenade into the right fist — LIVE in PIE, grenade equipped.

The grenade hold pose is a runtime Kinemation pose (one-handed ready), so
unlike the pipe it cannot be sampled offline from an AnimSequence. Run this
while PIE is up and the grenade is in hand.

Same grip model that fixed the pipe (session 63): channel center = centroid
of the curled finger-joint ring (NOT the wrist→knuckle line — metacarpals run
under the BACK of the hand). Single fist, so the cylinder axis = the
across-fist line from index knuckle to pinky knuckle.

Applies to the LIVE grenade actor (instant visual check), writes the
BP_GraceCharacter CDO, and prints values for ZP_KinemationComponent.h.
NOTE: the CDO save will fail while PIE runs (UE refuses saves in PIE) — the
script warns; re-save after exiting PIE.
"""
import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = es.get_game_world()
assert world, 'NOT IN PIE — start PIE and equip the grenade first'
pawn = unreal.GameplayStatics.get_player_character(world, 0)
assert pawn, 'NO PAWN'

pm = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent)
          if c.get_name() == 'PlayerMesh')
grenade = None
for a in pawn.get_attached_actors():
    if 'Grenade' in a.get_name():
        grenade = a
        break
assert grenade, 'No attached actor with "Grenade" in the name — equip the grenade first'

mesh_comp = next((c for c in grenade.get_components_by_class(unreal.StaticMeshComponent)
                  if c.get_editor_property('static_mesh')), None)
assert mesh_comp, 'Grenade has no StaticMeshComponent with a mesh'
sm = mesh_comp.get_editor_property('static_mesh')

ws = unreal.RelativeTransformSpace.RTS_WORLD
GRIP_BONES = ([f'{f}_{j:02d}_r' for f in ('index', 'middle', 'ring', 'pinky') for j in (1, 2, 3)]
              + ['thumb_02_r', 'thumb_03_r'])

# Dev corrections (session 63): first fit came out upside down and buried in
# the knuckles. FLIP reverses the long axis; PALM_PULL slides the grenade out
# of the knuckle ring back toward the palm/wrist (cm).
FLIP_LONG_AXIS = True
PALM_PULL = 3.0

sx = sy = sz = 0.0
n = 0
for b in GRIP_BONES:
    try:
        t = pm.get_socket_transform(b, ws).translation
        sx += t.x
        sy += t.y
        sz += t.z
        n += 1
    except Exception:
        pass
assert n >= 8, f'only {n} grip bones found on PlayerMesh hand_r'
center = unreal.Vector(sx / n, sy / n, sz / n)

# Across-fist axis: index knuckle → pinky knuckle (a cylinder held in one
# fist lies along this line).
i1 = pm.get_socket_transform('index_01_r', ws).translation
p1 = pm.get_socket_transform('pinky_01_r', ws).translation
dn = unreal.Vector(p1.x - i1.x, p1.y - i1.y, p1.z - i1.z).normal()
if FLIP_LONG_AXIS:
    dn = unreal.Vector(-dn.x, -dn.y, -dn.z)

# Pull out of the knuckle ring toward the palm: the grenade is fatter than
# the ring, so its center sits deeper in the hand than the joint centroid.
hr_pos = pm.get_socket_transform('hand_r', ws).translation
toward_palm = unreal.Vector(hr_pos.x - center.x, hr_pos.y - center.y, hr_pos.z - center.z).normal()
center = unreal.Vector(center.x + toward_palm.x * PALM_PULL,
                       center.y + toward_palm.y * PALM_PULL,
                       center.z + toward_palm.z * PALM_PULL)

# Grenade long axis from bounds + pivot correction (geometry center, not
# pivot, lands on the channel center).
ext = sm.get_bounds().box_extent
borig = sm.get_bounds().origin
axes = {'x': ext.x, 'y': ext.y, 'z': ext.z}
long_axis = max(axes, key=axes.get)
unreal.log(f'grenade mesh: {sm.get_name()}, long axis {long_axis}, extents ({ext.x:.1f}, {ext.y:.1f}, {ext.z:.1f})')
if long_axis == 'x':
    rotw = unreal.MathLibrary.make_rot_from_x(dn)
elif long_axis == 'y':
    rotw = unreal.MathLibrary.make_rot_from_y(dn)
else:
    rotw = unreal.MathLibrary.make_rot_from_z(dn)

tf = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=rotw, scale=unreal.Vector(1, 1, 1))
ro = tf.transform_direction(borig)
pivot = unreal.Vector(center.x - ro.x, center.y - ro.y, center.z - ro.z)
desired = unreal.Transform(location=pivot, rotation=rotw, scale=unreal.Vector(1, 1, 1))

hr = pm.get_socket_transform('hand_r', ws)
rel = unreal.MathLibrary.make_relative_transform(desired, hr)
rel_rot = rel.rotation.rotator()

# Apply to the live actor for instant visual check
grenade.set_actor_relative_location(rel.translation, False, False)
grenade.set_actor_relative_rotation(rel_rot, False, False)

unreal.log('=== GRENADE GRIP (bake into ZP_KinemationComponent.h) ===')
unreal.log(f'ThrowableGripOffset   = FVector({rel.translation.x:.2f}f, {rel.translation.y:.2f}f, {rel.translation.z:.2f}f)')
unreal.log(f'ThrowableGripRotation = FRotator({rel_rot.pitch:.2f}f, {rel_rot.yaw:.2f}f, {rel_rot.roll:.2f}f)')

# Write the CDO (save will fail during PIE — warned below, re-save after)
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
kin = next(c for c in cdo.get_components_by_class(unreal.ActorComponent) if 'Kinemation' in c.get_name())
kin.set_editor_property('AZP_ThrowableGripOffset', rel.translation)
kin.set_editor_property('AZP_ThrowableGripRotation', rel_rot)
saved = unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
if saved:
    unreal.log('APPLIED to BP_GraceCharacter CDO and SAVED to disk.')
else:
    unreal.log_warning('APPLIED to in-memory CDO but SAVE FAILED (expected during PIE) — re-save after exiting PIE!')
