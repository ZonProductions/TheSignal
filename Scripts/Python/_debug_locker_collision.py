"""Debug: dump ALL components on a placed BP_LootLocker and their collision settings."""
import unreal

actors = unreal.EditorLevelLibrary.get_all_level_actors()
lockers = [a for a in actors if 'LootLocker' in a.get_class().get_name()]

if not lockers:
    print("No BP_LootLocker found in level!")
else:
    actor = lockers[0]
    print(f"Inspecting: {actor.get_actor_label()}")

    # Get ALL components
    all_comps = actor.get_components_by_class(unreal.ActorComponent)
    print(f"\nTotal components: {len(all_comps)}")

    for comp in all_comps:
        name = comp.get_name()
        cls = comp.get_class().get_name()

        # Check if it's a primitive (has collision)
        if isinstance(comp, unreal.PrimitiveComponent):
            collision_enabled = comp.get_editor_property('collision_enabled') if hasattr(comp, 'collision_enabled') else 'unknown'
            collision_profile = comp.get_collision_profile_name()
            generates_overlap = comp.get_editor_property('generate_overlap_events') if hasattr(comp, 'generate_overlap_events') else 'unknown'

            # Check collision response to Pawn channel
            try:
                pawn_response = comp.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)
            except:
                pawn_response = 'unknown'

            print(f"\n  [{cls}] {name}")
            print(f"    CollisionProfile: {collision_profile}")
            print(f"    PawnResponse: {pawn_response}")

            if isinstance(comp, unreal.StaticMeshComponent):
                mesh = comp.static_mesh
                print(f"    StaticMesh: {mesh.get_name() if mesh else 'None'}")
                visible = comp.is_visible()
                print(f"    Visible: {visible}")

            if isinstance(comp, unreal.SphereComponent):
                radius = comp.sphere_radius
                print(f"    Radius: {radius}")
        else:
            print(f"\n  [{cls}] {name} (non-primitive)")
