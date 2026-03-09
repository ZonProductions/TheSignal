"""Revert both slots to MI_ColorShinyBlackPlastic, then only change slot 1 to glass."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
black_mat = unreal.load_asset("/Game/office_BigCompanyArchViz/Materials/MI_ColorShinyBlackPlastic")
glass_mat = unreal.load_asset("/Game/CharacterCustomizer/Base_Materials/Material_Instances/MI_Glass_AO")

print(f"Black mat: {black_mat is not None}, Glass mat: {glass_mat is not None}")

# Need to find the actual path for MI_ColorShinyBlackPlastic if the above fails
if not black_mat:
    # Try to get it from a window wall that still has glass on it
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == 'F4_SM_WindowWall10':
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            # Slot 3 is MI_ColorDarkMetallic, untouched - check slot 0
            mat0 = smc.get_material(0)
            print(f"  Current slot 0: {mat0.get_name() if mat0 else 'None'}")
            print(f"  Current slot 0 path: {mat0.get_path_name() if mat0 else 'None'}")
            break

fixed = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'WindowWall' not in label:
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    # Restore slot 0 (frame) to black, keep slot 1 as glass
    if black_mat:
        smc.set_material(0, black_mat)
    fixed += 1

print(f"Restored frame (slot 0) on {fixed} WindowWall actors")
