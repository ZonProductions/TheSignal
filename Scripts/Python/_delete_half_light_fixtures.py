import unreal

# Delete every other light fixture mesh. Power's out — half the ceiling panels
# are dark/missing. Sells the containment breach atmosphere.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

LIGHT_KEYWORDS = ['light', 'lamp']

# Collect all light fixture static mesh actors, grouped by mesh type
fixtures_by_mesh = {}
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name().lower()
    if any(kw in mesh_name for kw in LIGHT_KEYWORDS):
        if mesh_name not in fixtures_by_mesh:
            fixtures_by_mesh[mesh_name] = []
        fixtures_by_mesh[mesh_name].append(a)

deleted = 0
kept = 0
for mesh_name, actors in fixtures_by_mesh.items():
    # Delete every other one (index 0, 2, 4... kept; 1, 3, 5... deleted)
    for i, actor in enumerate(actors):
        if i % 2 == 1:
            actor.destroy_actor()
            deleted += 1
        else:
            kept += 1

print(f"Deleted {deleted} light fixtures, kept {kept}")

subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print("Level saved.")
