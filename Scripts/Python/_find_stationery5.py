import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
for a in actors:
    name = a.get_actor_label()
    if name == "F5_stationery_seperat_polySurface242":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps:
            mesh = c.get_editor_property("static_mesh")
            if mesh:
                unreal.log("Mesh path: " + str(mesh.get_path_name()))
                unreal.log("Mesh name: " + str(mesh.get_name()))
            else:
                unreal.log("static_mesh is None")
            # Try getting mesh from the component directly
            unreal.log("Component: " + c.get_name())
        break
