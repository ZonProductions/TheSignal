import unreal
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
print("CURRENT MAP:", ues.get_editor_world().get_name())

# Can we construct the destination struct?
try:
    d = unreal.ZP_TransitDestination()
    print("struct class OK:", type(d).__name__)
    d.set_editor_property("destination_id", "Floor3")
    d.set_editor_property("display_name", unreal.Text("Floor 3 - Labs"))
    d.set_editor_property("arrival_point_tag", "ElevatorArrival")
    d.set_editor_property("target_level", unreal.SoftObjectPath("/Game/Campaign/Building1_3rdFloor.Building1_3rdFloor"))
    print("fields set OK; target_level =", d.get_editor_property("target_level"))
except Exception as e:
    print("STRUCT ERR:", repr(e))

# Is the transit panel class available?
print("panel class:", unreal.ZP_TransitPanel)
print("menu class:", unreal.ZP_TransitMenuWidget)
