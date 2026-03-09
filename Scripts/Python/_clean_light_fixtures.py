import unreal

# Kill the swimming pool glow:
# 1. Delete ALL non-fluorescent light fixtures (SM_LampBig is the main offender)
# 2. Thin out fluorescents — keep every 3rd one for sparse horror lighting

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

FLUORESCENT_MESH = 'SM_Florosent'
LIGHT_KEYWORDS = ['light', 'lamp']

deleted_fixtures = 0
deleted_fluorescents = 0
kept_fluorescents = 0

# Collect fluorescents per floor for thinning
fluorescents_by_floor = {}

for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    mesh_name = smc.static_mesh.get_name()

    if mesh_name == FLUORESCENT_MESH:
        # Group by floor prefix for per-floor thinning
        label = a.get_actor_label()
        floor = label.split('_')[0] if '_' in label else 'F1'
        if floor not in fluorescents_by_floor:
            fluorescents_by_floor[floor] = []
        fluorescents_by_floor[floor].append(a)
    elif any(kw in mesh_name.lower() for kw in LIGHT_KEYWORDS):
        # Delete ALL non-fluorescent light fixtures
        a.destroy_actor()
        deleted_fixtures += 1

# Thin fluorescents: keep every 3rd one per floor
for floor, actors in fluorescents_by_floor.items():
    for i, actor in enumerate(actors):
        if i % 3 == 0:
            kept_fluorescents += 1
        else:
            actor.destroy_actor()
            deleted_fluorescents += 1

print(f"Deleted {deleted_fixtures} non-fluorescent fixtures (SM_LampBig, etc.)")
print(f"Deleted {deleted_fluorescents} fluorescents (thinned to every 3rd)")
print(f"Kept {kept_fluorescents} fluorescents (sparse)")

subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print("Level saved.")
