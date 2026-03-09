import unreal
import os

# Import Building 1.wav as a Sound Wave and place a looping AmbientSound actor in the level.

src_path = 'C:/Users/Ommei/workspace/TheSignal/Building1.wav'
dest_path = '/Game/TheSignal/Audio/Music'
asset_name = 'SFX_Building1_Ambience'

# --- Import the WAV ---
task = unreal.AssetImportTask()
task.set_editor_property('filename', src_path)
task.set_editor_property('destination_path', dest_path)
task.set_editor_property('destination_name', asset_name)
task.set_editor_property('replace_existing', True)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

# Verify import
imported = unreal.EditorAssetLibrary.load_asset(f'{dest_path}/{asset_name}')
if not imported:
    print(f'FAILED to import {src_path}')
else:
    print(f'Imported: {dest_path}/{asset_name}')

    # Set looping on the Sound Wave
    imported.set_editor_property('looping', True)
    unreal.EditorAssetLibrary.save_asset(f'{dest_path}/{asset_name}')
    print('  Looping = True')

    # Place an AmbientSound actor in the level
    subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Delete existing one if present (re-run safe)
    for a in subsys.get_all_level_actors():
        if a.get_actor_label() == 'AmbientSound_Building1Music':
            a.destroy_actor()
            print('  Removed old AmbientSound_Building1Music')

    ambient = subsys.spawn_actor_from_class(unreal.AmbientSound, unreal.Vector(0, 0, 0))
    ambient.set_actor_label('AmbientSound_Building1Music')

    # Set the sound on the audio component
    audio_comp = ambient.get_component_by_class(unreal.AudioComponent)
    if audio_comp:
        audio_comp.set_editor_property('sound', imported)
        audio_comp.set_editor_property('auto_activate', True)
        # Make it 2D (heard everywhere equally) by setting attenuation to none
        audio_comp.set_editor_property('override_attenuation', True)
        attenuation = audio_comp.get_editor_property('attenuation_overrides')
        attenuation.set_editor_property('b_attenuation', False)
        audio_comp.set_editor_property('attenuation_overrides', attenuation)
        print('  AudioComponent: sound set, auto-activate ON, non-spatialized (2D)')

    # Save level
    subsys_level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsys_level.save_all_dirty_levels()
    print('Level saved. AmbientSound_Building1Music placed at origin.')
    print('Music will auto-play and loop when entering PIE.')
