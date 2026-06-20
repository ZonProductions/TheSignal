import unreal, math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
def yaw(v): return round(math.degrees(math.atan2(v.y, v.x)),1)
caps = pawn.get_components_by_class(unreal.CapsuleComponent)[0]
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
gm = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="CharacterMesh0")
KR = unreal.AttachmentRule.KEEP_RELATIVE
mb.attach_to_component(caps, "", KR, KR, KR, False)
mb.set_relative_location(unreal.Vector(0.0,0.0,-90.0), False, False)
mb.set_relative_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), False, False)
print("actor fwd yaw:", yaw(pawn.get_actor_forward_vector()))
print("CharacterMesh0 (correct ref) fwd yaw:", yaw(gm.get_forward_vector()), "up.z:", round(gm.get_up_vector().z,2))
print("MarcusBody fwd yaw:", yaw(mb.get_forward_vector()), "up.z:", round(mb.get_up_vector().z,2))
