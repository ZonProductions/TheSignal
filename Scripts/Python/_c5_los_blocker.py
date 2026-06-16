import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ploc = pc.get_controlled_pawn().get_actor_location()
c5 = next((a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor) if "PatrolCreature_C_5" in a.get_name()), None)
cloc = c5.get_actor_location()
wallN = unreal.Vector(0,-1,0)
start = cloc + unreal.Vector(0,0,40) + wallN*100
end = ploc + unreal.Vector(0,0,40)
unreal.log_warning("C5=%s  player=%s  start(off)=%s" % (cloc, ploc, start))
hit = unreal.SystemLibrary.line_trace_single_for_objects(world, start, end,
    [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [c5], unreal.DrawDebugTrace.FOR_DURATION, True)
if hit:
    # impact_point is gettable via the struct's get_editor_property fallback chain
    for prop in ["impact_point","location"]:
        try:
            ip = hit.get_editor_property(prop); unreal.log_warning("BLOCKED %s=%s" % (prop, ip)); break
        except Exception: pass
    try:
        comp = hit.get_editor_property("hit_object_handle")
        unreal.log_warning("blocker handle=%s" % comp)
    except Exception as e:
        unreal.log_warning("handle err %s" % e)
else:
    unreal.log_warning("CLEAR (no blocker) - LOS should be true!")
# Also: is the player roughly on the wall's open side? dot of (player-c5) with wallN
to_player = (ploc - cloc); to_player_n = to_player.normal()
unreal.log_warning("dot(toPlayer, wallN)=%.2f (positive=player on open side)" % to_player_n.dot(wallN))
