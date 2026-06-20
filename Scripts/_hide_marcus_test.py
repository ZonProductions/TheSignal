import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
hid=[]
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    n=c.get_name()
    if n.startswith("Marcus"):
        c.set_visibility(False, True); hid.append(n)
print("hid:", hid)
