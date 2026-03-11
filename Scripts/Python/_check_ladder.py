import unreal

# Find all ladder actors in the current level
actors = unreal.EditorLevelLibrary.get_all_level_actors()
ladders = [a for a in actors if 'Ladder' in a.get_class().get_name() or 'Ladder' in a.get_name()]

print(f"Found {len(ladders)} ladder-related actors:")
for a in ladders:
    print(f"  {a.get_name()} — class: {a.get_class().get_name()}")

    # Check for BoxComponent
    box_comps = a.get_components_by_class(unreal.BoxComponent)
    print(f"    BoxComponents: {len(box_comps)}")
    for bc in box_comps:
        print(f"      {bc.get_name()} — generates_overlap={bc.get_generate_overlap_events()} extent={bc.get_scaled_box_extent()} loc={bc.get_world_location()}")

    # Check for StaticMeshComponent
    mesh_comps = a.get_components_by_class(unreal.StaticMeshComponent)
    print(f"    StaticMeshComponents: {len(mesh_comps)}")
    for mc in mesh_comps:
        sm = mc.get_editor_property('static_mesh')
        print(f"      {mc.get_name()} — mesh={sm.get_name() if sm else 'NONE'}")

    # Check IZP_Interactable interface
    implements = a.get_class().implement_interface(unreal.load_object(None, '/Script/TheSignal.ZP_Interactable'))
    print(f"    Implements IZP_Interactable: {implements}")

if not ladders:
    print("NO LADDER ACTORS FOUND IN LEVEL")
