"""Revert BOTH slots 0 and 1 back to MI_ColorShinyBlackPlastic on all WindowWall actors."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
black_mat = unreal.load_asset("/Game/office_BigCompanyArchViz/Materials/MI_ColorShinyBlackPlastic")

fixed = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'WindowWall' not in label:
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc or not black_mat:
        continue
    smc.set_material(0, black_mat)
    smc.set_material(1, black_mat)
    fixed += 1

print(f"Reverted slots 0 AND 1 to MI_ColorShinyBlackPlastic on {fixed} WindowWall actors")
