import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
# force another rebuild + try a SHORT path near the floor (lower Z toward actual floor ~990)
unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
a = unreal.Vector(-600,-400,990)
b = unreal.Vector(0,-400,990)
try:
    path = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, a, b)
    unreal.log_warning("short PATH valid=%s pts=%d" % (path.is_valid() if path else None,
        len(path.path_points) if path and hasattr(path,'path_points') else -1))
except Exception as e:
    unreal.log_warning("path err: %s" % e)
