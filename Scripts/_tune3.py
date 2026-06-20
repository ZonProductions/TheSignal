import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
gc = next(c for c in pawn.get_components_by_class(unreal.ActorComponent) if "GraceGameplay" in c.get_class().get_name())
gc.set_editor_property("CameraExtraForward", -60.0)
gc.set_editor_property("CameraExtraHeight", 3.0)
print("forward -60, height 3")
