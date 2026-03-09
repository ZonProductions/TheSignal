import unreal

# SH2-style darkness: cut all light intensities to 20% of current values.
# The flashlight should be the player's primary light source.
# Ambient lights provide just enough to see shapes/silhouettes, not details.

INTENSITY_SCALE = 0.20

all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
lights = [a for a in all_actors if isinstance(a, unreal.Light)]

changed = 0
for light in lights:
    comp = light.light_component
    old_intensity = comp.intensity
    new_intensity = old_intensity * INTENSITY_SCALE
    comp.set_editor_property('intensity', new_intensity)
    print(f"  {light.get_actor_label()}: {old_intensity:.1f} -> {new_intensity:.1f}")
    changed += 1

print(f"\nDarkened {changed} lights to {INTENSITY_SCALE*100:.0f}% intensity")

# Save the level
unreal.EditorLevelLibrary.save_all_dirty_levels()
print("Level saved.")
