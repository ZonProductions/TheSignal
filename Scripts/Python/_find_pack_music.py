import unreal

# Search for all sound-related assets that might be auto-playing from the map pack.
# Check: level blueprint, audio volumes, any actor with audio that activates on BeginPlay.

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

print("=== ALL actors with AudioComponent (auto-activate check) ===")
count = 0
for a in all_actors:
    comps = a.get_components_by_class(unreal.AudioComponent)
    for comp in comps:
        auto_act = comp.get_editor_property('auto_activate')
        sound = comp.get_editor_property('sound')
        sound_name = sound.get_name() if sound else 'None'
        sound_path = sound.get_path_name() if sound else 'N/A'
        vol = comp.get_editor_property('volume_multiplier')
        label = a.get_actor_label()
        class_name = a.get_class().get_name()
        print(f'  [{class_name}] {label}')
        print(f'    Sound: {sound_name} ({sound_path})')
        print(f'    AutoActivate: {auto_act}, Volume: {vol}')
        count += 1

print(f'\nTotal: {count} AudioComponents found')

# Also search for any AudioVolume actors
print('\n=== AudioVolume actors ===')
for a in all_actors:
    if 'AudioVolume' in a.get_class().get_name():
        print(f'  {a.get_actor_label()} ({a.get_class().get_name()})')

# Check for any BP actors that might spawn audio
print('\n=== Blueprint actors (potential audio spawners) ===')
for a in all_actors:
    class_name = a.get_class().get_name()
    if class_name.startswith('BP_') and 'sound' in class_name.lower() or 'music' in class_name.lower() or 'audio' in class_name.lower():
        print(f'  [{class_name}] {a.get_actor_label()}')
