import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    unreal.log_warning("NO PIE WORLD")
else:
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = pc.get_controlled_pawn()
    ploc = pawn.get_actor_location()
    unreal.log_warning("PLAYER at %s" % ploc)
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if "PatrolCreature" not in a.get_name():
            continue
        cloc = a.get_actor_location()
        dist = cloc.distance(ploc)
        # Replicate HasClearLOS: WorldStatic trace crawler+40 -> player+40, ignore crawler
        s = cloc + unreal.Vector(0,0,40)
        e = ploc + unreal.Vector(0,0,40)
        hit = unreal.SystemLibrary.line_trace_single(world, s, e,
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
        # Note: TraceTypeQuery1=Visibility; do a WorldStatic object trace instead:
        hitWS = unreal.SystemLibrary.line_trace_single_for_objects(world, s, e,
            [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [a], unreal.DrawDebugTrace.NONE, True)
        blocked = "CLEAR"
        if hitWS:
            d = hitWS.get_editor_property("distance")
            ip = hitWS.get_editor_property("impact_point")
            blocked = "BLOCKED at dist=%.0f impact=%s (fullDist=%.0f)" % (d, ip, s.distance(e))
        unreal.log_warning("  %s Z=%.0f distToPlayer=%.0f LOS=%s" % (a.get_name(), cloc.z, dist, blocked))
