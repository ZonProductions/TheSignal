import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
loc = cam.get_world_location()
fwd = cam.get_forward_vector()
# find actors whose bounding box contains the camera point
print("CAM loc:", round(loc.x,1), round(loc.y,1), round(loc.z,1))
enclosing=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
    o,e = a.get_actor_bounds(False)
    if abs(loc.x-o.x)<e.x and abs(loc.y-o.y)<e.y and abs(loc.z-o.z)<e.z:
        smc=a.static_mesh_component
        sm=smc.static_mesh.get_name() if (smc and smc.static_mesh) else None
        enclosing.append((a.get_actor_label(), sm, round((o-loc).length())))
print("StaticMeshActors enclosing camera:", enclosing[:15])
# trace forward (visibility)
hit = unreal.SystemLibrary.line_trace_single(world, loc, loc+fwd*150, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [pawn], unreal.DrawDebugTrace.NONE, True)
if hit:
    ha=hit.get_editor_property('hit_actor') if hasattr(hit,'get_editor_property') else None
    print("fwd trace hit:", ha.get_actor_label() if ha else hit.hit_actor)
else:
    print("fwd trace: no hit")
