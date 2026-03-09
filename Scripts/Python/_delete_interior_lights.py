import unreal

# Delete ALL SpotLight and RectLight actors. Power is out — containment breach.
# Keep: SkyAtmosphere, ExponentialHeightFog, LightmassPortals, LightmassImportanceVolume

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

deleted = 0
for a in all_actors:
    if isinstance(a, (unreal.SpotLight, unreal.RectLight, unreal.PointLight)):
        label = a.get_actor_label()
        a.destroy_actor()
        print(f"  Deleted: {label}")
        deleted += 1

print(f"\nDeleted {deleted} interior lights")

subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsys_level.save_all_dirty_levels()
print("Level saved.")
