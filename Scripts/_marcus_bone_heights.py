import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = None
for c in pawn.get_components_by_class(unreal.CameraComponent):
    cam = c
camloc = cam.get_world_location()
print(f"CAMERA world: ({round(camloc.x,1)},{round(camloc.y,1)},{round(camloc.z,1)})")
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
print("MarcusBody comp world:", mb.get_world_location())
import math
camfwd = cam.get_forward_vector()
for b in ["head","spine_03","spine_01","pelvis","thigh_l","calf_l","foot_l","ball_l"]:
    try:
        p = mb.get_bone_location(b) if hasattr(mb,"get_bone_location") else mb.get_socket_location(b)
    except Exception as e:
        print(b, "ERR", e); continue
    d = p - camloc
    fwd = round(d.x*camfwd.x + d.y*camfwd.y + d.z*camfwd.z,1)  # along view
    print(f"  {b}: world=({round(p.x,1)},{round(p.y,1)},{round(p.z,1)}) dZ_from_cam={round(d.z,1)} dist={round(d.length(),1)} alongView={fwd}")
