import unreal

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()
npc = [a for a in actors if 'BP_NPC' in a.get_class().get_name()][0]

print('=== BP_NPC Apparel State ===')

# Check preset
preset = npc.get_editor_property('Preset')
print(f'Preset: {preset.get_name() if preset else None}')
if preset:
    default_apparel = preset.get_editor_property('Default Apparel')
    print(f'Preset Default Apparel: {default_apparel}')

# Check actor-level apparel options
try:
    apparel_opts = npc.get_editor_property('Apparel Options')
    print(f'Actor Apparel Options: {apparel_opts}')
except Exception as e:
    print(f'Apparel Options: {e}')

try:
    apparel_mats = npc.get_editor_property('Apparel Materials')
    print(f'Actor Apparel Materials: {apparel_mats}')
except Exception as e:
    print(f'Apparel Materials: {e}')

# Check Character Load Option
clo = npc.get_editor_property('Character Load Option')
print(f'Character Load Option: {clo}')

# Check Local Data
local = npc.get_editor_property('Local Data')
print(f'Local Data type: {type(local)}')

# Try to read Local Data fields
for prop in ['ApparelNames', 'Apparel', 'Preset', 'SaveName', 'Save Name']:
    try:
        val = local.get_editor_property(prop)
        print(f'  Local Data.{prop}: {val}')
    except:
        pass

# Check the other load option
try:
    CLO = type(clo)
    print(f'\nAvailable load options:')
    print(f'  DO_NOT_LOAD_ANYTHING = {CLO.DO_NOT_LOAD_ANYTHING}')
    print(f'  LOAD_PRESET_DEFAULT_DATA = {CLO.LOAD_PRESET_DEFAULT_DATA}')
    print(f'  CUSTOMIZE_IN_EDITOR = {CLO.CUSTOMIZE_IN_EDITOR}')
except:
    pass
