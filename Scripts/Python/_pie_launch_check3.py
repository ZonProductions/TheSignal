import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ploc = pc.get_controlled_pawn().get_actor_location()
unreal.log_warning("PLAYER at (%.0f,%.0f,%.0f) LurkRange=1500" % (ploc.x, ploc.y, ploc.z))
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "PatrolCreature" not in a.get_name():
        continue
    cloc = a.get_actor_location()
    dist = cloc.distance(ploc)
    s = cloc + unreal.Vector(0,0,40)
    e = ploc + unreal.Vector(0,0,40)
    full = s.distance(e)
    hit = unreal.SystemLibrary.line_trace_single_for_objects(world, s, e,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [a], unreal.DrawDebugTrace.NONE, True)
    if hit:
        r = unreal.SystemLibrary.break_hit_result(hit)
        # break order: blocking_hit, initial_overlap, time, distance, location, impact_point, normal, impact_normal, phys_mat, hit_actor, ...
        hd = r[3]; ha = r[9]
        los = "BLOCKED dist=%.0f/%.0f by %s -> %s" % (hd, full, (ha.get_name() if ha else "?"),
            "wall-between" if hd < full-60 else "(reaches player)")
    else:
        los = "CLEAR (full=%.0f)" % full
    inr = "IN" if dist <= 1500 else "OUT>1500"
    unreal.log_warning("  %s Z=%.0f dist=%.0f [%s] LOS=%s" % (a.get_name(), cloc.z, dist, inr, los))
