"""Swap all MI_Glass material references to MI_Glass_AO in the current level."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Find materials by scanning all loaded assets with the name
glass_old = None
glass_new = None

# Use EditorAssetLibrary to search
eal = unreal.EditorAssetLibrary
all_assets = eal.list_assets('/Game/', recursive=True, include_folder=False)

for path in all_assets:
    if path.endswith('MI_Glass_AO') or '/MI_Glass_AO.' in path:
        glass_new = unreal.load_asset(path)
        print(f"Found MI_Glass_AO: {path}")
    elif (path.endswith('MI_Glass') or '/MI_Glass.' in path) and 'MI_Glass_AO' not in path:
        glass_old = unreal.load_asset(path)
        print(f"Found MI_Glass: {path}")

if not glass_old:
    print("ERROR: Could not find MI_Glass")
if not glass_new:
    print("ERROR: Could not find MI_Glass_AO")

if not glass_old or not glass_new:
    raise RuntimeError("Missing materials")

# Scan all mesh components in the level
swapped = 0
actors_modified = set()

for actor in all_actors:
    sm_comps = actor.get_components_by_class(unreal.StaticMeshComponent)
    for comp in sm_comps:
        num_mats = comp.get_num_materials()
        for i in range(num_mats):
            mat = comp.get_material(i)
            if mat and mat.get_name() == 'MI_Glass':
                comp.set_material(i, glass_new)
                swapped += 1
                actors_modified.add(actor.get_actor_label())

    sk_comps = actor.get_components_by_class(unreal.SkeletalMeshComponent)
    for comp in sk_comps:
        num_mats = comp.get_num_materials()
        for i in range(num_mats):
            mat = comp.get_material(i)
            if mat and mat.get_name() == 'MI_Glass':
                comp.set_material(i, glass_new)
                swapped += 1
                actors_modified.add(actor.get_actor_label())

print(f"\n=== Swapped {swapped} material slots across {len(actors_modified)} actors ===")
for name in sorted(actors_modified):
    print(f"  {name}")
