import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
recast = [a for a in eas.get_all_level_actors() if "RecastNavMesh" in a.get_class().get_name()]
unreal.log_warning("RecastNavMesh actors: %d" % len(recast))
# Test a projection + path between two crawler-area points
p1 = unreal.Vector(-2801,-1737,1077); p2 = unreal.Vector(1561,950,1077)
nav = unreal.NavigationSystemV1.get_navigation_system(world) if hasattr(unreal.NavigationSystemV1,'get_navigation_system') else None
try:
    proj = unreal.NavigationSystemV1.project_point_to_navigation(world, p1)
    unreal.log_warning("project p1 -> %s" % proj)
except Exception as e:
    unreal.log_warning("project err: %s" % e)
try:
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, p1, p2)
    if path:
        unreal.log_warning("PATH valid=%s points=%d" % (path.is_valid(), len(path.path_points) if hasattr(path,'path_points') else -1))
    else:
        unreal.log_warning("PATH none")
except Exception as e:
    unreal.log_warning("path err: %s" % e)
