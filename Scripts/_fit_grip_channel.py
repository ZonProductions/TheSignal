"""Thread the pipe through the GRIP CHANNEL of both fists.
Channel = inside the curled fingers: knuckle ridge pulled back toward the palm.
"""
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
p = unreal.GameplayStatics.get_player_character(w, 0)
mv = next(c for c in p.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'MeleeViewMesh')
pipe = next(c for c in p.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'MeleeViewWeapon')
ws = unreal.RelativeTransformSpace.RTS_WORLD

CURL = 0.62  # how far from palm toward knuckle ridge the channel center sits


def channel(side):
    hand = mv.get_socket_transform(f'hand_{side}', ws).translation
    ridge = unreal.Vector(0, 0, 0)
    n = 0
    for f in ['index_01', 'middle_01', 'ring_01', 'pinky_01']:
        try:
            b = mv.get_socket_transform(f'{f}_{side}', ws).translation
            ridge = unreal.Vector(ridge.x + b.x, ridge.y + b.y, ridge.z + b.z)
            n += 1
        except Exception:
            pass
    ridge = unreal.Vector(ridge.x / n, ridge.y / n, ridge.z / n)
    return unreal.Vector(hand.x + (ridge.x - hand.x) * CURL,
                         hand.y + (ridge.y - hand.y) * CURL,
                         hand.z + (ridge.z - hand.z) * CURL)


cR = channel('r')
cL = channel('l')
hr = mv.get_socket_transform('hand_r', ws)

d = unreal.Vector(cR.x - cL.x, cR.y - cL.y, cR.z - cL.z)
dist = d.length()
dn = d.normal()

# Shaft through both channels; geometric center placed so the butt clears the
# rear (left) fist by ~12cm. SM_Pipe: half-length 60.2 on Z, pivot offset.
HALF = 60.2
sm = pipe.get_editor_property('static_mesh')
borig = sm.get_bounds().origin
along = HALF - (dist + 12.0)
center = unreal.Vector(cR.x + dn.x * along, cR.y + dn.y * along, cR.z + dn.z * along)
rotw = unreal.MathLibrary.make_rot_from_z(dn)
rq = unreal.Transform(location=unreal.Vector(0, 0, 0), rotation=rotw, scale=unreal.Vector(1, 1, 1))
ro = rq.transform_direction(borig)
pivot = unreal.Vector(center.x - ro.x, center.y - ro.y, center.z - ro.z)
desired = unreal.Transform(location=pivot, rotation=rotw, scale=unreal.Vector(1, 1, 1))
rel = unreal.MathLibrary.make_relative_transform(desired, hr)
pipe.set_relative_location(rel.translation, False, False)
pipe.set_relative_rotation(rel.rotation.rotator(), False, False)

r = rel.rotation.rotator()
unreal.log(f'channel R: ({cR.x:.1f},{cR.y:.1f},{cR.z:.1f})  L: ({cL.x:.1f},{cL.y:.1f},{cL.z:.1f})  dist {dist:.1f}')
unreal.log(f'=== CHANNEL GRIP === loc ({rel.translation.x:.2f}, {rel.translation.y:.2f}, {rel.translation.z:.2f}) rot (p{r.pitch:.2f}, y{r.yaw:.2f}, r{r.roll:.2f})')

# markers: pink = channel centers, white = shaft
unreal.SystemLibrary.draw_debug_sphere(w, cR, 1.8, 10, unreal.LinearColor(1, 0.2, 0.8, 1), 15.0, 0.4)
unreal.SystemLibrary.draw_debug_sphere(w, cL, 1.8, 10, unreal.LinearColor(1, 0.2, 0.8, 1), 15.0, 0.4)
