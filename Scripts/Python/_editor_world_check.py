import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
print("editor world name:", w.get_name() if w else None)
print("editor world path:", w.get_path_name() if w else None)

# Is a PIE / game world active?
try:
    gi = ues.get_game_world()
    print("game(PIE) world:", gi.get_name() if gi else None)
except Exception as e:
    print("get_game_world err:", e)

# Level actor count via subsystem (what we've been using)
asub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
print("EditorActorSubsystem actor count:", len(asub.get_all_level_actors()))

# Current level path
try:
    lvl = unreal.LevelEditorSubsystem
    lsub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    cl = lsub.get_current_level()
    print("current level:", cl.get_path_name() if cl else None)
except Exception as e:
    print("level subsystem err:", e)
