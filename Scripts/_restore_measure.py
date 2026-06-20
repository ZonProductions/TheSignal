import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    c.set_hidden_in_game(False, True)
    if c.get_name()=="MarcusBody": c.set_relative_scale3d(unreal.Vector(1,1,1))
    # restore intended visibility: pawn body meshes visible to owner except PlayerMesh(ownernosee) & CharacterMesh0(hidden loco) & MeleeViewMesh(off)
    n=c.get_name()
    c.set_visibility(n.startswith("Marcus"), True)
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
camz = cam.get_world_location().z
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
hz = mb.get_socket_location("head").z
fz = mb.get_socket_location("foot_l").z
floor = fz
eye = hz - 12.0
print("camZ", round(camz,1), "headZ", round(hz,1), "footZ", round(fz,1))
print("eye above floor:", round(eye-floor,1), "| camera above floor:", round(camz-floor,1))
s = (camz-floor)/(eye-floor)
print("needed uniform scale to put eyes at camera:", round(s,3))
