import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
pm = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="PlayerMesh")
pm.set_owner_no_see(False)
print("PlayerMesh OwnerNoSee -> False (now rendered for owner). Watch Marcus's feet.")
