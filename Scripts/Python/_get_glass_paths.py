"""Get actual asset paths from F5 glass panels."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'F5_line_box_glass3':
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        sm = smc.get_editor_property("static_mesh")
        mat = smc.get_material(0)
        print(f"Mesh: {sm.get_path_name()}")
        print(f"Mat: {mat.get_path_name()}")
        break
