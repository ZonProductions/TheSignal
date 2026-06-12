"""Draw hand_r local axes + finger bones + pipe shaft in the PIE world."""
import unreal
es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
p = unreal.GameplayStatics.get_player_character(w, 0)
mv = next(c for c in p.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name() == 'MeleeViewMesh')
pipe = next(c for c in p.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == 'MeleeViewWeapon')
ws = unreal.RelativeTransformSpace.RTS_WORLD
hr = mv.get_socket_transform('hand_r', ws)
o = hr.translation
D = 18.0
T = 25.0


def axis(vec_local, color):
    d = hr.transform_direction(vec_local)
    end = unreal.Vector(o.x + d.x * D, o.y + d.y * D, o.z + d.z * D)
    unreal.SystemLibrary.draw_debug_line(w, o, end, color, T, 0.6)


axis(unreal.Vector(1, 0, 0), unreal.LinearColor(1, 0, 0, 1))   # X red
axis(unreal.Vector(0, 1, 0), unreal.LinearColor(0, 1, 0, 1))   # Y green
axis(unreal.Vector(0, 0, 1), unreal.LinearColor(0, 0.4, 1, 1)) # Z blue

for bone, col in [('middle_01_r', unreal.LinearColor(1, 0.6, 0, 1)),
                  ('middle_03_r', unreal.LinearColor(1, 1, 0, 1)),
                  ('thumb_03_r', unreal.LinearColor(0.8, 0, 1, 1)),
                  ('hand_l', unreal.LinearColor(0, 1, 1, 1)),
                  ('middle_01_l', unreal.LinearColor(0, 0.6, 0.6, 1))]:
    try:
        bp_ = mv.get_socket_transform(bone, ws).translation
        unreal.SystemLibrary.draw_debug_sphere(w, bp_, 1.5, 10, col, T, 0.4)
    except Exception:
        unreal.log_error(f'no bone {bone}')

# Pipe shaft line: local Z through pivot
pt = pipe.get_world_transform()
pz = pt.transform_direction(unreal.Vector(0, 0, 1))
a = unreal.Vector(pt.translation.x - pz.x * 60, pt.translation.y - pz.y * 60, pt.translation.z - pz.z * 60)
b = unreal.Vector(pt.translation.x + pz.x * 60, pt.translation.y + pz.y * 60, pt.translation.z + pz.z * 60)
unreal.SystemLibrary.draw_debug_line(w, a, b, unreal.LinearColor(1, 1, 1, 1), T, 0.3)
rl = pipe.get_relative_transform()
unreal.log(f'pipe rel loc: ({rl.translation.x:.1f}, {rl.translation.y:.1f}, {rl.translation.z:.1f})')
unreal.log('DRAWN: RGB=hand_r XYZ axes, orange=middle knuckle, yellow=middle tip, purple=thumb tip, cyan=left hand, white=pipe shaft')
