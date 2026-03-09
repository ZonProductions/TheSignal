import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

print(f"Total actors: {len(all_actors)}")

light_actors = []
sky_actors = []
fog_actors = []

for a in all_actors:
    cls = type(a).__name__
    label = a.get_actor_label()

    if 'Light' in cls:
        light_actors.append((label, cls))
    elif 'Sky' in cls:
        sky_actors.append((label, cls))
    elif 'Fog' in cls:
        fog_actors.append((label, cls))

print(f"\n--- Light actors ({len(light_actors)}) ---")
for name, cls in light_actors:
    print(f"  {name} ({cls})")

print(f"\n--- Sky actors ({len(sky_actors)}) ---")
for name, cls in sky_actors:
    print(f"  {name} ({cls})")

print(f"\n--- Fog actors ({len(fog_actors)}) ---")
for name, cls in fog_actors:
    print(f"  {name} ({cls})")
