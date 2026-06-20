import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
gc=None
for c in pawn.get_components_by_class(unreal.ActorComponent):
    if "GraceGameplay" in c.get_class().get_name(): gc=c
print("GameplayComp:", gc.get_name() if gc else None)
gc.set_editor_property("CameraExtraHeight", 20.0)
gc.set_editor_property("CameraExtraForward", -12.0)
print("set height +20, forward -12")
