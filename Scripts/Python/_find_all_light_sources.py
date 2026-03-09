import unreal

# Find EVERYTHING that could be emitting light — lights, emissive meshes, skylights, etc.
all_actors = unreal.EditorLevelLibrary.get_all_level_actors()

print(f"Total actors: {len(all_actors)}")

# Check for any light-related actors
light_actors = []
sky_actors = []
fog_actors = []
ppv_actors = []
other_light = []

for a in all_actors:
    cls = type(a).__name__
    label = a.get_actor_label()

    if 'Light' in cls or 'light' in label.lower():
        light_actors.append((label, cls))
    elif 'Sky' in cls or 'sky' in label.lower():
        sky_actors.append((label, cls))
    elif 'Fog' in cls or 'fog' in label.lower():
        fog_actors.append((label, cls))
    elif 'PostProcess' in cls:
        ppv_actors.append((label, cls))

print(f"\n--- Light actors ({len(light_actors)}) ---")
for name, cls in light_actors:
    print(f"  {name} ({cls})")

print(f"\n--- Sky actors ({len(sky_actors)}) ---")
for name, cls in sky_actors:
    print(f"  {name} ({cls})")

print(f"\n--- Fog actors ({len(fog_actors)}) ---")
for name, cls in fog_actors:
    print(f"  {name} ({cls})")

print(f"\n--- PostProcess volumes ({len(ppv_actors)}) ---")
for name, cls in ppv_actors:
    print(f"  {name} ({cls})")
