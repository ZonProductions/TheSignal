import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    n=c.get_name()
    c.set_hidden_in_game(False, True)
    if n=="PlayerMesh":
        c.set_visibility(True, False); c.set_owner_no_see(True)
    elif n=="CharacterMesh0":
        c.set_visibility(False, False)
    elif n=="MeleeViewMesh":
        c.set_visibility(False, False)
    elif n.startswith("Marcus"):
        c.set_visibility(True, False)
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
mb.set_relative_scale3d(unreal.Vector(0.869,0.869,0.869))
print("applied scale 0.869, restored visibility")
