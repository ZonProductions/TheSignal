"""Editor-viewport screenshots of the building exterior to see the blackbox.
Two shots staged via slate ticks (camera move needs a frame before capture)."""
import unreal

SHOTS = [
    ('bb_1_north_face', unreal.Vector(-1300, 9000, 2200),
     unreal.Rotator(roll=0, pitch=-8, yaw=-90)),
    ('bb_2_east_corner', unreal.Vector(8000, 7000, 3000),
     unreal.Rotator(roll=0, pitch=-12, yaw=-140)),
    ('bb_3_above', unreal.Vector(-1300, -500, 9000),
     unreal.Rotator(roll=0, pitch=-85, yaw=0)),
]

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
idx = [0]
frames = [0]

def tick(dt):
    frames[0] += 1
    if frames[0] % 30 != 0:
        return
    i = idx[0]
    if i >= len(SHOTS) * 2:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.log('[shot] done')
        return
    if i % 2 == 0:
        name, loc, rot = SHOTS[i // 2]
        ues.set_level_viewport_camera_info(loc, rot)
        unreal.log(f'[shot] staged {name}')
    else:
        name = SHOTS[i // 2][0]
        unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, f'{name}.png')
    idx[0] += 1

handle = unreal.register_slate_post_tick_callback(tick)
unreal.log('[shot] started')
