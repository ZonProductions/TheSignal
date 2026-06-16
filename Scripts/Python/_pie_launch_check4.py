import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ploc = pc.get_controlled_pawn().get_actor_location()
unreal.log_warning("PLAYER (%.0f,%.0f,%.0f) LurkRange=1500 DetectRange=1000" % (ploc.x, ploc.y, ploc.z))
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "PatrolCreature" not in a.get_name():
        continue
    cloc = a.get_actor_location()
    dist = cloc.distance(ploc)
    s = cloc + unreal.Vector(0,0,40)
    e = ploc + unreal.Vector(0,0,40)
    # WorldStatic-only trace: player isn't WorldStatic, so ANY hit = wall geometry between them.
    blockedWS = unreal.SystemLibrary.line_trace_single_for_objects(world, s, e,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [a], unreal.DrawDebugTrace.FOR_DURATION, True)
    inr = "IN-range" if dist <= 1500 else "OUT(>1500)"
    los = "LOS-BLOCKED(no launch)" if blockedWS else "LOS-CLEAR(should launch)"
    unreal.log_warning("  %s Z=%.0f dist=%.0f [%s] %s" % (a.get_name(), cloc.z, dist, inr, los))
