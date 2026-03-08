"""Inspect DecalActor8 — get its material and properties."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

for actor in all_actors:
    if actor.get_actor_label() == 'DecalActor8':
        print(f"Found: {actor.get_actor_label()}")
        print(f"Class: {actor.get_class().get_name()}")
        loc = actor.get_actor_location()
        print(f"Location: ({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f})")

        # Get the decal component
        comps = actor.get_components_by_class(unreal.DecalComponent)
        for comp in comps:
            print(f"DecalComponent: {comp.get_name()}")
            mat = comp.get_decal_material()
            if mat:
                print(f"  Material: {mat.get_name()}")
                print(f"  Material path: {mat.get_path_name()}")
            else:
                print("  Material: None")

            size = comp.decal_size
            print(f"  Decal size: ({size.x:.0f}, {size.y:.0f}, {size.z:.0f})")
        break
