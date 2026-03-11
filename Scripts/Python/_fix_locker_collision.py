"""Fix collision on ALL placed BP_LootLocker instances.
Nuclear option: NoCollision profile + ignore all channels."""
import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
lockers = [a for a in actors if 'LootLocker' in a.get_class().get_name()]
print(f"Found {len(lockers)} BP_LootLocker instances")

channels = [
    unreal.CollisionChannel.ECC_WORLD_STATIC,
    unreal.CollisionChannel.ECC_WORLD_DYNAMIC,
    unreal.CollisionChannel.ECC_PAWN,
    unreal.CollisionChannel.ECC_PHYSICS_BODY,
    unreal.CollisionChannel.ECC_VEHICLE,
    unreal.CollisionChannel.ECC_DESTRUCTIBLE,
]

for actor in lockers:
    label = actor.get_actor_label()
    comps = actor.get_components_by_class(unreal.PrimitiveComponent)
    for comp in comps:
        name = comp.get_name()
        cls = comp.get_class().get_name()
        if isinstance(comp, unreal.StaticMeshComponent):
            comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            comp.set_collision_profile_name('NoCollision')
            for ch in channels:
                comp.set_collision_response_to_channel(ch, unreal.CollisionResponseType.ECR_IGNORE)
            verify_pawn = comp.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)
            print(f"  {label}/{name}: NoCollision, Pawn={verify_pawn}")

print("\nDone — try PIE now")
