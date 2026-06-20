import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world,0)
pawn = pc.get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
cl = cam.get_world_location()
cr = cam.get_world_rotation() if hasattr(cam,'get_world_rotation') else cam.get_world_transform().rotator()
ctrl = pc.get_control_rotation()
af = pawn.get_actor_forward_vector(); ar = pawn.get_actor_right_vector(); au = pawn.get_actor_up_vector()
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
head = mb.get_socket_location("head")
d = cl - head
fwd = d.x*af.x + d.y*af.y + d.z*af.z
rgt = d.x*ar.x + d.y*ar.y + d.z*ar.z
up  = d.x*au.x + d.y*au.y + d.z*au.z
print("CAMERA vs HEAD (actor-local): forward=%.1f right=%.1f up=%.1f" % (fwd,rgt,up))
print("control yaw=%.1f pitch=%.1f | camera yaw=%.1f pitch=%.1f roll=%.1f" % (
    ctrl.yaw, ctrl.pitch, cr.yaw if hasattr(cr,'yaw') else cr.rotator().yaw, 0,0))
