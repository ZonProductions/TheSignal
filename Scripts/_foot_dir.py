import unreal, math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
def yaw2(a,b): d=b-a; return round(math.degrees(math.atan2(d.y,d.x)),1)
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
cf=cam.get_forward_vector(); camyaw=round(math.degrees(math.atan2(cf.y,cf.x)),1)
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
fl=mb.get_socket_location("foot_l"); bl=mb.get_socket_location("ball_l")
fr=mb.get_socket_location("foot_r"); br=mb.get_socket_location("ball_r")
print("camera yaw:", camyaw)
print("LEFT toe dir yaw:", yaw2(fl,bl), " RIGHT toe dir yaw:", yaw2(fr,br))
print("delta L:", round(yaw2(fl,bl)-camyaw,1), " delta R:", round(yaw2(fr,br)-camyaw,1))
