import unreal

# Delete ALL remaining fluorescent fixtures. The emissive material is still
# causing Lumen bounce / swimming pool effect. We'll start from zero and
# hand-place only what's needed later.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

deleted = 0
for a in all_actors:
    if not isinstance(a, unreal.StaticMeshActor):
        continue
    smc = a.static_mesh_component
    if not smc or not smc.static_mesh:
        continue
    if smc.static_mesh.get_name() == 'SM_Florosent':
        label = a.get_actor_label()
        a.destroy_actor()
        print(f"  Deleted: {label}")
        deleted += 1

print(f"\nDeleted {deleted} fluorescents — zero emissive fixtures remain")

subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print("Level saved.")
