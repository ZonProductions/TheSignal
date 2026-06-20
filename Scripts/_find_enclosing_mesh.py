import unreal
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
pawn = unreal.GameplayStatics.get_player_controller(world,0).get_controlled_pawn()
cam = pawn.get_components_by_class(unreal.CameraComponent)[0]
loc = cam.get_world_location()
found=[]
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if a==pawn: continue
    for mc in a.get_components_by_class(unreal.MeshComponent):
        try:
            b = mc.bounds  # FBoxSphereBounds
            o = b.origin; r = b.sphere_radius
            d = (o-loc).length()
            if d < r and r < 600:   # camera inside a smallish mesh's bounds
                asset=None
                if isinstance(mc, unreal.StaticMeshComponent) and mc.static_mesh: asset=mc.static_mesh.get_name()
                elif isinstance(mc, unreal.SkeletalMeshComponent):
                    sm=mc.get_skeletal_mesh_asset() if hasattr(mc,'get_skeletal_mesh_asset') else None
                    asset=sm.get_name() if sm else None
                found.append((round(d),round(r),a.get_actor_label(),type(mc).__name__,asset))
        except Exception: pass
found.sort()
for f in found[:20]: print(f)
