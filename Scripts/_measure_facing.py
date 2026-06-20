import unreal, math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world,0)
pawn = pc.get_controlled_pawn()
def yaw(v): return round(math.degrees(math.atan2(v.y, v.x)),1)
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
camf = cam.get_forward_vector()
camyaw = round(math.degrees(math.atan2(camf.y, camf.x)),1)
print("control yaw:", round(pc.get_control_rotation().yaw,1))
print("actor yaw:", round(pawn.get_actor_rotation().yaw,1))
print("camera fwd yaw (horiz):", camyaw)
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
# CCMH visual forward = +Y axis = component right vector
print("MarcusBody +Y(forward) yaw:", yaw(mb.get_right_vector()))
print("--> body should equal actor/camera yaw; delta = how far feet point off")
