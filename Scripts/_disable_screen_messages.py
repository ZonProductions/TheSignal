import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "DisableAllScreenMessages")
print("DisableAllScreenMessages executed — all on-screen viewport messages muted.")
print("Reversible with: EnableAllScreenMessages")
