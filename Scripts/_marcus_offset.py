import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
aloc = pawn.get_actor_location()
af = pawn.get_actor_forward_vector(); ar = pawn.get_actor_right_vector()
print("ACTOR loc:", round(aloc.x,1), round(aloc.y,1), round(aloc.z,1), "| fwd", round(af.x,2),round(af.y,2))
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
gm = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="CharacterMesh0")
for name,comp in [("MarcusBody",mb),("CharacterMesh0",gm)]:
    p = comp.get_socket_location("pelvis")
    d = p - aloc
    fwd = d.x*af.x + d.y*af.y   # forward of actor
    rgt = d.x*ar.x + d.y*ar.y   # right of actor
    print(f"  {name} pelvis: world=({round(p.x,1)},{round(p.y,1)}) | fwdOfActor={round(fwd,1)} rightOfActor={round(rgt,1)}")
print("MarcusBody rel loc:", mb.get_relative_location() if hasattr(mb,'get_relative_location') else mb.relative_location)
