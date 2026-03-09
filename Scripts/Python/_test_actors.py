"""Quick test: count actors using both APIs."""
import unreal

# API 1
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors1 = eas.get_all_level_actors()
print(f"EditorActorSubsystem: {len(actors1)} actors")

# API 2
actors2 = unreal.EditorLevelLibrary.get_all_level_actors()
print(f"EditorLevelLibrary: {len(actors2)} actors")

# Check world
world = unreal.EditorLevelLibrary.get_editor_world()
print(f"Editor world: {world}")
print(f"World name: {world.get_name() if world else 'None'}")
