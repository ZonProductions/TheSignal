import unreal

asset_path = '/Game/TheSignal/Audio/Music/SFX_Building1_Ambience'

sound = unreal.load_asset(asset_path)
if not sound:
    print(f'Could not load {asset_path}')
else:
    print(f'Loaded: {sound.get_name()} ({sound.get_class().get_name()})')

    # Set looping
    sound.set_editor_property('looping', True)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    print('  Looping = True, saved.')

    subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Remove old one if re-running
    for a in subsys.get_all_level_actors():
        if a.get_actor_label() == 'AmbientSound_Building1Music':
            a.destroy_actor()
            print('  Removed old AmbientSound_Building1Music')

    # Spawn a generic actor, then find existing AmbientSound or use level scripting approach
    # Try spawning via EditorLevelLibrary
    actor_class = unreal.AmbientSound.static_class()
    print(f'  AmbientSound class: {actor_class}')
    ambient = subsys.spawn_actor_from_class(actor_class, unreal.Vector(0, 0, 0))
    print(f'  Spawned: {ambient}')

    if ambient:
        ambient.set_actor_label('AmbientSound_Building1Music')
        audio_comp = ambient.get_component_by_class(unreal.AudioComponent)
        if audio_comp:
            audio_comp.set_editor_property('sound', sound)
            audio_comp.set_editor_property('auto_activate', True)
            audio_comp.set_editor_property('override_attenuation', True)
            attenuation = audio_comp.get_editor_property('attenuation_overrides')
            attenuation.set_editor_property('b_attenuation', False)
            audio_comp.set_editor_property('attenuation_overrides', attenuation)
            audio_comp.set_editor_property('volume_multiplier', 0.4)
            print('  AudioComponent: sound set, auto-activate, 2D, volume 0.4')

        subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        subsys_level.save_all_dirty_levels()
        print('Done. Level saved.')
    else:
        print('  FAILED to spawn AmbientSound. Place manually:')
        print('  1. Place AmbientSound actor in level')
        print('  2. Set Sound to SFX_Building1_Ambience')
        print('  3. Enable Auto Activate, set Volume Multiplier to 0.4')
        print('  4. Override Attenuation, disable Attenuation (makes it 2D)')
