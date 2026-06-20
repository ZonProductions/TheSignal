import unreal, math
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = ues.get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = pc.get_controlled_pawn()
def yaw(v): return round(math.degrees(math.atan2(v.y, v.x)), 1)
af = pawn.get_actor_forward_vector()
print("ACTOR fwd yaw:", yaw(af))
for c in pawn.get_components_by_class(unreal.SkeletalMeshComponent):
    n = c.get_name()
    if n in ("MarcusBody","PlayerMesh","CharacterMesh0"):
        f = c.get_forward_vector()
        r = c.get_right_vector()
        sm = c.get_skeletal_mesh_asset() if hasattr(c,"get_skeletal_mesh_asset") else None
        print(f"  {n}: fwd_yaw={yaw(f)} right_yaw={yaw(r)} mesh={sm.get_name() if sm else None}")
print("NOTE: visible body should FACE actor fwd. Whichever of the component's axes")
print("points at ACTOR fwd yaw tells us the mesh's true facing axis.")
