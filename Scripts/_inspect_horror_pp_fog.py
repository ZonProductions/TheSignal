import unreal

# Current editor state
w = unreal.EditorLevelLibrary.get_editor_world()
print("CURRENT_LEVEL:", w.get_name() if w else None)
dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
print("DIRTY_MAP_PACKAGES:", [p.get_name() for p in dirty])

# Load the source world asset (read-only inspection)
src = unreal.load_asset("/Game/Variant_Horror/Lvl_Horror")
print("SRC_WORLD:", src)

# Try to enumerate actors of the loaded world. For OFPA this may be empty
# unless the level is the editor world, so report what we can see.
try:
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    ppvs = [a for a in actors if isinstance(a, unreal.PostProcessVolume)]
    fogs = [a for a in actors if isinstance(a, unreal.ExponentialHeightFog)]
    print("EDITOR_WORLD_PPV_COUNT:", len(ppvs), "FOG_COUNT:", len(fogs))
except Exception as e:
    print("ENUM_ERR:", e)
