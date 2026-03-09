import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsys.get_all_level_actors()

print("=== Audio-related actors in level ===")
for a in all_actors:
    class_name = a.get_class().get_name()
    label = a.get_actor_label()
    # Check for AmbientSound, audio components, or anything sound-related
    if 'sound' in class_name.lower() or 'audio' in class_name.lower() or 'ambient' in class_name.lower() or 'music' in class_name.lower():
        print(f'  [{class_name}] {label}')
        continue
    # Also check if any actor has an AudioComponent
    audio_comp = a.get_component_by_class(unreal.AudioComponent)
    if audio_comp:
        sound = audio_comp.get_editor_property('sound')
        sound_name = sound.get_name() if sound else 'None'
        print(f'  [{class_name}] {label} — AudioComponent with sound: {sound_name}')
