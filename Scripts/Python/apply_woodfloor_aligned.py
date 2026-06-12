"""
Apply M_WoodFloorAligned to every slot currently using MI_Floor in
Maps_BigCompany (session 63 floor pass — pairs with build_woodfloor_aligned.py).

The 1x1m FloorTile grid + filler pieces all share MI_Floor; the world-aligned
material makes them read as one continuous wood surface.

Does NOT save the level — dev look-checks first, then save.
Re-run safe: already-swapped slots no longer match MI_Floor.
"""
import unreal

NEW_MAT = unreal.load_asset('/Game/Core/Materials/M_WoodFloorAligned')
assert NEW_MAT, 'M_WoodFloorAligned not found — run build_woodfloor_aligned.py first'

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
assert 'BigCompany' in w.get_name(), f'wrong map loaded: {w.get_name()}'

swapped = 0
actors = set()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.StaticMeshActor):
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if m and m.get_name() == 'MI_Floor':
                c.modify()
                c.set_material(i, NEW_MAT)
                swapped += 1
                actors.add(a.get_actor_label())

unreal.log(f'[woodfloor] swapped {swapped} slots on {len(actors)} actors (MI_Floor -> M_WoodFloorAligned)')
