import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
ploc = pc.get_controlled_pawn().get_actor_location()
unreal.log_warning("PLAYER at %s (LurkRange=1500)" % ploc)
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if "PatrolCreature" not in a.get_name():
        continue
    cloc = a.get_actor_location()
    dist = cloc.distance(ploc)
    s = cloc + unreal.Vector(0,0,40)
    e = ploc + unreal.Vector(0,0,40)
    full = s.distance(e)
    hitWS = unreal.SystemLibrary.line_trace_single_for_objects(world, s, e,
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], False, [a], unreal.DrawDebugTrace.NONE, True)
    if hitWS:
        ip = hitWS.get_editor_property("impact_point")
        hd = s.distance(ip)
        los = "BLOCKED at %.0f / %.0f (clear=%s)" % (hd, full, "yes" if hd > full-60 else "NO-wall-between")
    else:
        los = "CLEAR"
    inrange = "IN" if dist <= 1500 else "OUT(>1500)"
    unreal.log_warning("  %s Z=%.0f dist=%.0f [%s] LOS=%s" % (a.get_name(), cloc.z, dist, inrange, los))
