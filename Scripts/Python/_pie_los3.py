import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ploc = pc.get_controlled_pawn().get_actor_location()
crawler = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor) if "PatrolCreature_C_4" in a.get_name())
cloc = crawler.get_actor_location()
full = cloc.distance(ploc)
hit = unreal.SystemLibrary.line_trace_single(world, cloc, ploc, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [crawler], unreal.DrawDebugTrace.NONE, True)
if hit:
    blocking = hit.get_editor_property("blocking_hit")
    hd = hit.get_editor_property("distance")
    imp = hit.get_editor_property("impact_point")
    reached = hd > full - 60.0
    unreal.log_warning("BLOCKED blocking=%s traceLen=%.0f fullDist=%.0f impact=%s -> %s" % (
        blocking, hd, full, imp, "REACHED PLAYER (effectively clear)" if reached else "GEOMETRY BLOCKS LOS"))
else:
    unreal.log_warning("CLEAR fullDist=%.0f - open line of sight" % full)
