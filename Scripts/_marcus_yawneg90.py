import unreal, math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
def yaw(v): return round(math.degrees(math.atan2(v.y, v.x)),1)
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
mb.set_relative_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0), False, False)
print("actor fwd:", yaw(pawn.get_actor_forward_vector()), "| Marcus right_vec yaw:", yaw(mb.get_right_vector()), "up.z", round(mb.get_up_vector().z,2))
