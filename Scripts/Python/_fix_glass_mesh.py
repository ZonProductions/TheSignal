"""Fix the spawned glass panels with correct mesh and material."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

glass_mesh = unreal.load_asset("/Game/office_BigCompanyArchViz/StaticMesh/Probs/SM_FireExtinguisherBoxGlass")
glass_mat = unreal.load_asset("/Game/CharacterCustomizer/Base_Materials/Material_Instances/MI_Glass_AO")

print(f"Mesh loaded: {glass_mesh is not None}")
print(f"Mat loaded: {glass_mat is not None}")

fixed = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    # Match F1-F4 glass panels we just created
    if label in ('F1_line_box_glass3', 'F1_line_box_glass4',
                 'F2_line_box_glass3', 'F2_line_box_glass4',
                 'F3_line_box_glass3', 'F3_line_box_glass4',
                 'F4_line_box_glass3', 'F4_line_box_glass4'):
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            if glass_mesh:
                smc.set_static_mesh(glass_mesh)
            if glass_mat:
                smc.set_material(0, glass_mat)
            smc.set_editor_property('cast_shadow', False)
            fixed += 1
            print(f"  Fixed: {label}")

print(f"\n{fixed} panels fixed with correct mesh and material")
