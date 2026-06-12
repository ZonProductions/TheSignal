"""
REVERT (2026-06-12): undo the session-63 woodfloor pass in Maps_BigCompany.
Swaps every M_WoodFloorAligned slot back to the pack original MI_Floor,
then saves the level. Inverse of apply_woodfloor_aligned.py.
"""
import unreal

OLD_MAT = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_Floor')
assert OLD_MAT, 'MI_Floor not found — ABORT, nothing changed'

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
assert 'BigCompany' in w.get_name(), f'wrong map loaded: {w.get_name()}'

swapped = 0
actors = set()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and m.get_name() == 'M_WoodFloorAligned':
                c.modify()
                c.set_material(i, OLD_MAT)
                swapped += 1
                actors.add(a.get_actor_label())

unreal.log(f'[revert] {swapped} slots on {len(actors)} actors -> MI_Floor')

# verify nothing left on the new material
left = 0
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and m.get_name() == 'M_WoodFloorAligned':
                left += 1
unreal.log(f'[revert] remaining M_WoodFloorAligned slots: {left}')

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'[revert] level saved: {ok}')
