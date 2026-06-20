import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
loc = cam.get_world_location()
# overlap a small sphere at the camera to find what encloses it
hits = unreal.SystemLibrary.sphere_overlap_actors(world, loc, 25.0, [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY3, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY4, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY5], None, [pawn])
print("overlap actors @cam:", [a.get_actor_label() for a in hits])
# also: nearest skeletal/static mesh comps within 60cm and their meshes
import math
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    d = (a.get_actor_location()-loc).length()
    if d < 120 and a != pawn:
        comps = a.get_components_by_class(unreal.MeshComponent)
        for mc in comps:
            asset = None
            if isinstance(mc, unreal.StaticMeshComponent) and mc.static_mesh: asset=mc.static_mesh.get_name()
            elif isinstance(mc, unreal.SkeletalMeshComponent):
                sm=mc.get_skeletal_mesh_asset() if hasattr(mc,'get_skeletal_mesh_asset') else None
                asset=sm.get_name() if sm else None
            if asset:
                print(f"  near({round(d)}) {a.get_actor_label()} :: {mc.get_name()} = {asset}")
