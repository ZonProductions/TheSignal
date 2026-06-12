"""Editor screenshots from inside floor 3 looking at/through the north windows."""
import unreal

SHOTS = [
    ('bb_4_inside_window', unreal.Vector(0, 500, 1110),
     unreal.Rotator(roll=0, pitch=0, yaw=90)),
    ('bb_5_inside_room', unreal.Vector(-2000, 0, 1110),
     unreal.Rotator(roll=0, pitch=0, yaw=45)),
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
        unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, f'{SHOTS[i // 2][0]}.png')
    idx[0] += 1

handle = unreal.register_slate_post_tick_callback(tick)
unreal.log('[shot] started')
