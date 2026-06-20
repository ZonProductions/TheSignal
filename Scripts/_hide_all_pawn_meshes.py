import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    c.set_visibility(False, True)
    c.set_hidden_in_game(True, True)
    print("hid", c.get_name())
