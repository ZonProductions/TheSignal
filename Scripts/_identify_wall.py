import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
assert w, 'NOT IN PIE'
p = unreal.GameplayStatics.get_player_character(w, 0)
ploc = p.get_actor_location()

best = None
best_d = 1e9
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    if 'LootLocker' in a.get_class().get_name():
        d = (a.get_actor_location() - ploc).length()
        if d < best_d:
            best, best_d = a, d
target = best.get_actor_location()

# Object query (what bullets use) — finds the wall
obj_types = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]  # WorldStatic, WorldDynamic
hits = unreal.SystemLibrary.line_trace_multi_for_objects(w, ploc, target, obj_types, False, [p], unreal.DrawDebugTrace.FOR_DURATION, True)
unreal.log(f'OBJECT trace pawn->locker: {len(hits)} hits')
for h in hits:
    t = h.to_tuple()
    ha, hc = t[9], t[10]
    if not ha or 'LootLocker' in ha.get_name() or 'Volume' in ha.get_class().get_name():
        continue
    unreal.log(f'  WALL CANDIDATE: actor={ha.get_name()} class={ha.get_class().get_name()} comp={hc.get_name() if hc else None}')
    if hc:
        unreal.log(f'    profile={hc.get_collision_profile_name()}')
        unreal.log(f'    visibility response={hc.get_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY)}')
        unreal.log(f'    camera response={hc.get_collision_response_to_channel(unreal.CollisionChannel.ECC_CAMERA)}')
        unreal.log(f'    collision enabled={hc.get_collision_enabled()}')
    break
