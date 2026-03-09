import unreal

# Near-total darkness. Kill all interior lights to ~5% of their ORIGINAL values.
# Current values are already at 20% from last pass, so multiply by 0.25 to reach 5% original.
# Only moonlight through windows should provide ambient.

SCALE = 0.25  # 20% * 0.25 = 5% of original

all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
lights = [a for a in all_actors if isinstance(a, unreal.Light)]

changed = 0
for light in lights:
    comp = light.light_component
    old = comp.intensity
    new = old * SCALE
    comp.set_editor_property('intensity', new)
    changed += 1

print(f"Crushed {changed} lights to {SCALE*100:.0f}% (now ~5% of original)")

unreal.EditorLevelLibrary.save_all_dirty_levels()
print("Level saved.")
