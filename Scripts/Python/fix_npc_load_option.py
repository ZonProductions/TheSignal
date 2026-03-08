import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()

npcs = [a for a in actors if 'BP_NPC' in a.get_class().get_name() or 'CC_Customizable_NPC' in a.get_class().get_name()]
print(f'Found {len(npcs)} NPC actors')

if npcs:
    CLO = type(npcs[0].get_editor_property('Character Load Option'))
    target = CLO.LOAD_PRESET_DEFAULT_DATA
    print(f'Target value: {target}')

    for a in npcs:
        a.set_editor_property('Character Load Option', target)
        result = a.get_editor_property('Character Load Option')
        print(f'  {a.get_name()}: {result}')

# Also set the default on the BP_NPC class
bp = unreal.load_asset('/Game/Blueprints/Actors/BP_NPC')
if bp:
    cdo = bp.generated_class().get_default_object()
    try:
        cdo.set_editor_property('Character Load Option', target)
        print(f'BP_NPC CDO set to: {cdo.get_editor_property("Character Load Option")}')
    except Exception as e:
        print(f'CDO set failed: {e}')

unreal.EditorAssetLibrary.save_asset('/Game/Blueprints/Actors/BP_NPC')
print('Saved BP_NPC')
