import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
cl = cam.get_world_location()
print("CAMERA world:", round(cl.x,1), round(cl.y,1), round(cl.z,1))
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
for b in ["head","neck_01","spine_05","spine_03","pelvis","foot_l"]:
    p = mb.get_socket_location(b)
    d = p - cl
    print(f"  Marcus {b}: world=({round(p.x,1)},{round(p.y,1)},{round(p.z,1)}) | from cam dZ={round(d.z,1)} dXY={round((d.x**2+d.y**2)**0.5,1)}")
