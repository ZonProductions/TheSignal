"""4-stop PIE photo tour: where does the night appear/disappear?
Teleports the player + control rotation, screenshots each stop."""
import unreal, time

STOPS = [
    ('tour_1_inside_center', unreal.Vector(-11000, -400, 1060), 90),   # interior, face north
    ('tour_2_at_glass',      unreal.Vector(1572, 950, 1060),    90),   # 3m south of test pane, face it
    ('tour_3_outside_near',  unreal.Vector(1572, 2600, 120),    90),   # just past north face, ground, face away
    ('tour_4_outside_far',   unreal.Vector(1572, 8000, 170),   -90),   # 55m out, face back at building
]

gw = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
assert gw, 'PIE not running'
pc = unreal.GameplayStatics.get_player_controller(gw, 0)
pawn = unreal.GameplayStatics.get_player_pawn(gw, 0)

idx = [0]
def step(dt):
    i = idx[0]
    if i >= len(STOPS) * 2:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.log('[tour] done')
        return
    if i % 2 == 0:
        name, loc, yaw = STOPS[i // 2]
        pawn.set_actor_location(loc, False, False)
        pc.set_control_rotation(unreal.Rotator(roll=0, pitch=0, yaw=yaw))
        unreal.log(f'[tour] stop {i//2 + 1}: {name}')
    else:
        name = STOPS[i // 2][0]
        unreal.AutomationLibrary.take_high_res_screenshot(1280, 720, f'{name}.png')
    idx[0] += 1

# slate tick callback ~every frame; throttle with a frame counter
frames = [0]
def tick(dt):
    frames[0] += 1
    if frames[0] % 45 == 0:   # ~every 0.75s
        step(dt)

handle = unreal.register_slate_post_tick_callback(tick)
unreal.log('[tour] started')
