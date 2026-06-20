import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
mb.set_relative_scale3d(unreal.Vector(0.84,0.84,0.84))
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
camz = cam.get_world_location().z
for b in ["head","spine_03","pelvis","foot_l"]:
    z = mb.get_bone_location(b).z
    print(b, "Z", round(z,1), "rel cam", round(z-camz,1))
print("camera Z", round(camz,1))
