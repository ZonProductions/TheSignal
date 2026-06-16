import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = pc.get_controlled_pawn()
ploc = pawn.get_actor_location()
crawler = None
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "PatrolCreature_C_4" in a.get_name():
        crawler = a; break
cloc = crawler.get_actor_location()
unreal.log_warning("dist=%.0f  cloc=%s ploc=%s" % (cloc.distance(ploc), cloc, ploc))
hit = unreal.SystemLibrary.line_trace_single(
    world, cloc, ploc, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [crawler],
    unreal.DrawDebugTrace.NONE, True)
if hit:
    comp = hit.get_editor_property("component")
    owner = comp.get_owner() if comp else None
    unreal.log_warning("BLOCKED comp=%s owner=%s loc=%s dist=%.0f" % (
        comp.get_name() if comp else "None",
        owner.get_name() if owner else "None",
        hit.get_editor_property("location"),
        hit.get_editor_property("distance")))
else:
    unreal.log_warning("CLEAR - nothing blocks Visibility between crawler and player")
