import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
unreal.SystemLibrary.execute_console_command(w, "r.Shadow.Virtual.MaxPhysicalPages 8192")
val = unreal.SystemLibrary.get_console_variable_float_value("r.Shadow.Virtual.MaxPhysicalPages")
print("MaxPhysicalPages now =", val)
print("Look at the viewport — is the VSM overflow message gone?")
