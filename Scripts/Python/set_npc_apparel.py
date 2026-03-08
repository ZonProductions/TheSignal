import unreal
import re

sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = sub.get_all_level_actors()
npc = [a for a in actors if 'BP_NPC' in a.get_class().get_name()][0]

# Set to CUSTOMIZE_IN_EDITOR
CLO = type(npc.get_editor_property('Character Load Option'))
npc.set_editor_property('Character Load Option', CLO.CUSTOMIZE_IN_EDITOR)

# Get preset data
preset = npc.get_editor_property('Preset')
default_apparel = preset.get_editor_property('Default Apparel')
dt_apparel = preset.get_editor_property('DT Apparel')
print(f'Preset: {preset.get_name()}')

# Build DataTableRowHandle array
handles = []
for i, row_name in enumerate(default_apparel):
    if i < len(dt_apparel):
        # Extract clean path from full object path string
        dt_str = str(dt_apparel[i])
        match = re.search(r"'(.+?)'", dt_str)
        if match:
            clean_path = match.group(1)
        else:
            clean_path = dt_str

        dt = unreal.load_asset(clean_path)
        handle = unreal.DataTableRowHandle()
        handle.set_editor_property('DataTable', dt)
        handle.set_editor_property('RowName', str(row_name))
        handles.append(handle)
        print(f'  Slot {i}: {row_name} -> {dt.get_name() if dt else "FAILED: " + clean_path}')

npc.set_editor_property('Apparel Options', handles)
npc.set_editor_property('Apparel Materials', [0, 0, 0, 0])
print(f'Apparel Options set ({len(handles)} slots), Materials set')
print('Hit Play to test')
