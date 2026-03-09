"""Swap MI_ColorShinyBlackPlastic to MI_Glass_AO on all SM_WindowWall actors."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
glass_mat = unreal.load_asset("/Game/CharacterCustomizer/Base_Materials/Material_Instances/MI_Glass_AO")
print(f"MI_Glass_AO loaded: {glass_mat is not None}")

swapped = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'WindowWall' not in label:
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    for i in range(smc.get_num_materials()):
        mat = smc.get_material(i)
        if mat and mat.get_name() == 'MI_ColorShinyBlackPlastic':
            smc.set_material(i, glass_mat)
    swapped += 1

print(f"Swapped glass material on {swapped} WindowWall actors")
