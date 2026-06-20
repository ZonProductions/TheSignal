import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
loc = cam.get_world_location()
fwd = cam.get_forward_vector()
print("CAM", round(loc.x),round(loc.y),round(loc.z), "fwd", round(fwd.x,2),round(fwd.y,2),round(fwd.z,2))
rows=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if a==pawn: continue
    for mc in a.get_components_by_class(unreal.MeshComponent):
        try:
            b=mc.bounds; o=b.origin; r=b.sphere_radius
            d=(o-loc).length()
            rows.append((round(d-r), round(d), round(r), a.get_actor_label(), type(mc).__name__))
        except Exception: pass
rows.sort()
for x in rows[:12]: print(x)
