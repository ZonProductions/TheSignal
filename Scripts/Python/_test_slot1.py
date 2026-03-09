"""Test: set ONLY slot 1 to MI_Glass_AO on F4_SM_WindowWall10."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
glass_mat = unreal.load_asset("/Game/CharacterCustomizer/Base_Materials/Material_Instances/MI_Glass_AO")

for a in eas.get_all_level_actors():
    if a.get_actor_label() == 'F4_SM_WindowWall10':
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        smc.set_material(1, glass_mat)
        print("Set slot 1 to MI_Glass_AO on F4_SM_WindowWall10")
        break
