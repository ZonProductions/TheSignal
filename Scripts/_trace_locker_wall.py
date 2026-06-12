import unreal

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_game_world()
assert w, 'NOT IN PIE'
p = unreal.GameplayStatics.get_player_character(w, 0)
assert p, 'NO PAWN'
ploc = p.get_actor_location()

# Live wall-check flag on the player's Moonville interaction component
for c in p.get_components_by_class(unreal.ActorComponent):
    if 'InteractionComponent' in c.get_class().get_name():
        try:
            unreal.log(f'PreventInteractionThroughWall (live) = {c.get_editor_property("PreventInteractionThroughWall")}')
        except Exception as e:
            unreal.log(f'flag read failed: {e}')
        break

# Find the nearest loot locker
best = None
best_d = 1e9
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor):
    if 'LootLocker' in a.get_class().get_name() or 'LootLocker' in a.get_name():
        d = (a.get_actor_location() - ploc).length()
        if d < best_d:
            best, best_d = a, d
assert best, 'no LootLocker found'
unreal.log(f'nearest locker: {best.get_name()} at {best_d:.0f}cm')

# Its interaction collision location + true scaled radii
target = best.get_actor_location()
unreal.log(f'locker actor scale: {best.get_actor_scale3d()}')
for c in best.get_components_by_class(unreal.SphereComponent):
    unreal.log(f'  sphere comp {c.get_name()}: unscaled_radius={c.get_unscaled_sphere_radius():.0f} scaled_radius={c.get_scaled_sphere_radius():.0f} overlaps_player={c.is_overlapping_actor(p)}')
    if 'Collision' in c.get_name() or 'Interaction' in c.get_name():
        target = c.get_world_location()

# Trace pawn -> locker, visibility channel (what Moonville's wall check does)
hits = unreal.SystemLibrary.line_trace_multi(
    w, ploc, target, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    False, [p], unreal.DrawDebugTrace.FOR_DURATION, True)
unreal.log(f'trace pawn->locker: {len(hits)} hits')
for h in hits:
    ha = h.to_tuple()[9]
    hc = h.to_tuple()[10]
    unreal.log(f'  HIT: actor={ha.get_name() if ha else None} comp={hc.get_name() if hc else None} class={ha.get_class().get_name() if ha else None}')
