import unreal

all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
lights = [a for a in all_actors if isinstance(a, unreal.Light)]

print(f"Total lights: {len(lights)}")

# Group by intensity ranges
intensities = []
for light in lights:
    comp = light.light_component
    intensities.append((light.get_actor_label(), comp.intensity, type(comp).__name__))

intensities.sort(key=lambda x: -x[1])

print("\n--- All lights (brightest first) ---")
for name, intensity, ltype in intensities:
    print(f"  {name}: {intensity:.0f} ({ltype})")

# Summary
if intensities:
    vals = [x[1] for x in intensities]
    print(f"\nMin: {min(vals):.0f}, Max: {max(vals):.0f}, Avg: {sum(vals)/len(vals):.0f}")
