import unreal, math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
def yaw(v): return round(math.degrees(math.atan2(v.y, v.x)),1)
af = yaw(pawn.get_actor_forward_vector())
mb = next(c for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent) if c.get_name()=="MarcusBody")
mb.set_relative_rotation(unreal.Rotator(0.0,90.0,0.0), False, False)
print("actor fwd yaw:", af, "| MarcusBody fwd yaw after rel +90:", yaw(mb.get_forward_vector()))
