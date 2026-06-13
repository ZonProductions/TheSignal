import unreal

# Look for any Python-exposed way to toggle editor viewport realtime
for subsys_name, obj in [
    ("LevelEditorSubsystem", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)),
    ("UnrealEditorSubsystem", unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)),
]:
    hits = [m for m in dir(obj) if any(k in m.lower() for k in ("realtime", "real_time", "viewport"))]
    print(subsys_name, "->", hits)

# EditorLevelLibrary (deprecated but may expose it)
hits = [m for m in dir(unreal.EditorLevelLibrary) if any(k in m.lower() for k in ("realtime", "real_time", "viewport"))]
print("EditorLevelLibrary ->", hits)

# Any global library with 'viewport'/'realtime'
cands = [n for n in dir(unreal) if "Viewport" in n or "LevelEditor" in n]
print("classes:", cands)
