import unreal

sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level = sub.get_current_level()
print(f"Current level: {level}")
print(f"Level name: {level.get_name() if level else 'None'}")

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print(f"World: {world}")
if world:
    print(f"World name: {world.get_name()}")
