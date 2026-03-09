import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()
steps = [a for a in actors if a.get_class().get_name() == "StaticMeshActor"
         and a.static_mesh_component.static_mesh
         and "SM_Steps" in a.static_mesh_component.static_mesh.get_name()]

z_scale = 1.5

for a in steps:
    s = a.get_actor_scale3d()
    a.set_actor_scale3d(unreal.Vector(s.x, s.y, z_scale))
    print(f"  {a.get_actor_label()}: Z scale={z_scale}")

print(f"\nDone: {len(steps)} actors set to Z={z_scale}")
