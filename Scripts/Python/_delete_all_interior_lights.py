import unreal

# Delete ALL interior lights. Moonlight + flashlight only.
# The building is in a containment breach — power is out. No interior lights.

all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
lights = [a for a in all_actors if isinstance(a, unreal.Light)]

print(f"Found {len(lights)} lights — deleting all")

deleted = 0
for light in lights:
    label = light.get_actor_label()
    light.destroy_actor()
    deleted += 1

print(f"Deleted {deleted} lights")

unreal.EditorLevelLibrary.save_all_dirty_levels()
print("Level saved.")
