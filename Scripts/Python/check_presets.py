import unreal

preset_paths = [
    '/Game/CharacterCustomizer/Characters/CCMH/Presets/CCMH_Preset_Male_01',
    '/Game/CharacterCustomizer/Characters/CCMH/Presets/CCMH_Preset_Female_01',
    '/Game/CharacterCustomizer/Characters/CCMH/Presets/CCMH_Preset_Ommei',
]

for path in preset_paths:
    asset = unreal.load_asset(path)
    if asset:
        print(f'OK: {path} -> {asset.get_class().get_name()}')
        # Check key properties
        for prop in ['Gender', 'Body', 'Head', 'Default Apparel', 'DT Apparel']:
            try:
                val = asset.get_editor_property(prop)
                print(f'  {prop} = {val}')
            except:
                pass
    else:
        print(f'FAIL: {path} -> None')

# Also check if CC_Customizable_NPC loads
npc = unreal.load_asset('/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Customizable_NPC')
print(f'\nCC_Customizable_NPC: {npc}')
if npc:
    gen = npc.generated_class()
    cdo = gen.get_default_object()
    try:
        preset = cdo.get_editor_property('Preset')
        print(f'  Default Preset: {preset}')
    except Exception as e:
        print(f'  Preset property error: {e}')

# Check BP_NPC
bp = unreal.load_asset('/Game/Blueprints/Actors/BP_NPC')
print(f'\nBP_NPC: {bp}')
if bp:
    gen = bp.generated_class()
    cdo = gen.get_default_object()
    try:
        preset = cdo.get_editor_property('Preset')
        print(f'  Default Preset: {preset}')
    except Exception as e:
        print(f'  Preset property error: {e}')

# Check placed instance in level
print('\n--- Level actors ---')
actors = unreal.EditorLevelLibrary.get_all_level_actors()
for a in actors:
    if 'BP_NPC' in a.get_class().get_name() or 'CC_Customizable' in a.get_class().get_name():
        print(f'Found: {a.get_name()} ({a.get_class().get_name()})')
        try:
            p = a.get_editor_property('Preset')
            print(f'  Preset: {p}')
        except Exception as e:
            print(f'  Preset error: {e}')
        # Check components
        comps = a.get_components_by_class(unreal.SkeletalMeshComponent)
        for c in comps:
            mesh = c.get_editor_property('skeletal_mesh_asset') if hasattr(c, 'skeletal_mesh_asset') else None
            try:
                mesh = c.get_editor_property('SkinnedAsset')
            except:
                try:
                    mesh = c.get_editor_property('SkeletalMesh')
                except:
                    mesh = 'unknown'
            print(f'  SKMComp: {c.get_name()} -> mesh={mesh}')
