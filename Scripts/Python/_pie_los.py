import unreal
# find the PIE/game world
world = None
try:
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
except Exception as e:
    unreal.log_warning("no game world: %s" % e)
if not world:
    unreal.log_warning("PIE world NOT found (is PIE running?)"); 
else:
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = pc.get_controlled_pawn() if pc else None
    ploc = pawn.get_actor_location() if pawn else None
    crawler = None
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        if "PatrolCreature_C_4" in a.get_name():
            crawler = a; break
    if not crawler:
        # fall back: any patrol creature
        for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
            if "PatrolCreature" in a.get_name():
                crawler = a; break
    if not (crawler and ploc):
        unreal.log_warning("missing crawler=%s pawn=%s" % (crawler, pawn))
    else:
        cloc = crawler.get_actor_location()
        d = cloc - ploc
        unreal.log_warning("CRAWLER=%s cloc=%s ploc=%s dXYZ=(%.0f,%.0f,%.0f) dist=%.0f" % (
            crawler.get_name(), cloc, ploc, d.x, d.y, d.z, cloc.distance(ploc)))
        # line trace Visibility from crawler to player
        hit = unreal.SystemLibrary.line_trace_single(
            world, cloc, ploc, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [crawler],
            unreal.DrawDebugTrace.NONE, True)
        if hit:
            unreal.log_warning("LOS-TRACE BLOCKED by actor=%s comp=%s at %s" % (
                hit.get_actor().get_name() if hit.get_actor() else "None",
                hit.get_component().get_name() if hit.get_component() else "None",
                hit.location))
        else:
            unreal.log_warning("LOS-TRACE CLEAR (nothing between crawler and player)")
