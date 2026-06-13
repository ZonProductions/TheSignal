import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
print("LEVEL:", w.get_name() if w else None)
ok = les.save_current_level()
print("SAVED_LEVEL:", ok)
print("DIRTY_AFTER:", [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()])
