import unreal

# Find the actor by name in the current level
eas = unreal.EditorActorSubsystem()
actors = eas.get_all_level_actors()
for a in actors:
    name = a.get_actor_label()
    if "stationery" in name.lower() or "polySurface" in name or "F5_" in name:
        unreal.log("FOUND: " + name + " class=" + str(a.get_class().get_name()))
        # Get static mesh component
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            mesh = c.get_editor_property("static_mesh")
            if mesh:
                unreal.log("  Mesh: " + str(mesh.get_path_name()))
            mat = c.get_material(0)
            if mat:
                unreal.log("  Material[0]: " + str(mat.get_path_name()))
