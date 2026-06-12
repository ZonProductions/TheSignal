import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
assert w, 'NOT IN PIE'
p = unreal.GameplayStatics.get_player_character(w, 0)
ploc = p.get_actor_location()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
cam = pc.player_camera_manager.get_camera_location()

unreal.log(f'pawn at ({ploc.x:.0f}, {ploc.y:.0f}, {ploc.z:.0f}), camera at ({cam.x:.0f}, {cam.y:.0f}, {cam.z:.0f})')

# 1) What is the pawn overlapping right now?
overlapping = p.get_overlapping_actors()
for a in overlapping:
    unreal.log(f'PAWN OVERLAPS: {a.get_actor_label()} ({a.get_class().get_name()})')

# 2) Nearest locker
best = None
best_d = 1e9
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    if 'LootLocker' in a.get_class().get_name():
        d = (a.get_actor_location() - ploc).length()
        if d < best_d:
            best, best_d = a, d
target = None
for c in best.get_components_by_class(unreal.SphereComponent):
    if 'Collision' in c.get_name():
        target = c.get_world_location()
unreal.log(f'nearest locker {best.get_name()} at {best_d:.0f}cm')

# 3) Traces from pawn center AND camera, with blocking flags
for label, start in (('PAWN', ploc), ('CAMERA', cam)):
    hits = unreal.SystemLibrary.line_trace_multi(
        w, start, target, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False, [p], unreal.DrawDebugTrace.FOR_DURATION, True)
    unreal.log(f'{label} vis trace: {len(hits)} hits')
    for h in hits:
        t = h.to_tuple()
        blocking, ha, hc = t[0], t[9], t[10]
        unreal.log(f'  blocking={blocking} actor={ha.get_actor_label() if ha else None} comp={hc.get_name() if hc else None}')

# 4) Wall actors near the pawn: world box bounds + is pawn inside?
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    d = (a.get_actor_location() - ploc).length()
    if d > 600:
        continue
    label = a.get_actor_label()
    if 'wall' not in label.lower() and 'brick' not in label.lower():
        continue
    origin, extent = a.get_actor_bounds(False)
    inside = (abs(ploc.x - origin.x) <= extent.x and abs(ploc.y - origin.y) <= extent.y and abs(ploc.z - origin.z) <= extent.z)
    unreal.log(f'WALL {label}: bounds origin ({origin.x:.0f},{origin.y:.0f},{origin.z:.0f}) extent ({extent.x:.0f},{extent.y:.0f},{extent.z:.0f}) PAWN_INSIDE={inside}')
