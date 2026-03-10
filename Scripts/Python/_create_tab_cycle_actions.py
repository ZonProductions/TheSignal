"""
Create IA_TabCycleLeft and IA_TabCycleRight input actions,
then map them to Q and E in IMC_Grace.
Run via MCP: exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_create_tab_cycle_actions.py').read())
"""
import unreal

eal = unreal.EditorAssetLibrary
factory = unreal.InputAction_Factory()
af = unreal.AssetToolsHelpers.get_asset_tools()

# Create IA_TabCycleLeft (Q key)
ia_left_path = '/Game/Core/Input/Actions/IA_TabCycleLeft'
if not unreal.EditorAssetLibrary.does_asset_exist(ia_left_path):
    ia_left = af.create_asset('IA_TabCycleLeft', '/Game/Core/Input/Actions', unreal.InputAction, factory)
    ia_left.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(ia_left_path)
    unreal.log(f'Created {ia_left_path}')
else:
    ia_left = unreal.load_asset(ia_left_path)
    unreal.log(f'Already exists: {ia_left_path}')

# Create IA_TabCycleRight (E key)
ia_right_path = '/Game/Core/Input/Actions/IA_TabCycleRight'
if not unreal.EditorAssetLibrary.does_asset_exist(ia_right_path):
    ia_right = af.create_asset('IA_TabCycleRight', '/Game/Core/Input/Actions', unreal.InputAction, factory)
    ia_right.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(ia_right_path)
    unreal.log(f'Created {ia_right_path}')
else:
    ia_right = unreal.load_asset(ia_right_path)
    unreal.log(f'Already exists: {ia_right_path}')

# Map in IMC_Grace: Q → IA_TabCycleLeft, E → IA_TabCycleRight
imc = unreal.load_asset('/Game/Core/Input/IMC_Grace')
if imc:
    # Check if already mapped
    mappings = imc.get_editor_property('mappings')
    existing_actions = set()
    for m in mappings:
        action = m.get_editor_property('action')
        if action:
            existing_actions.add(action.get_name())

    if 'IA_TabCycleLeft' not in existing_actions:
        key_q = unreal.Key()
        key_q.import_text('Q')
        mapping_left = unreal.EnhancedActionKeyMapping()
        mapping_left.set_editor_property('action', ia_left)
        mapping_left.set_editor_property('key', key_q)
        mappings.append(mapping_left)
        unreal.log('Mapped Q -> IA_TabCycleLeft')
    else:
        unreal.log('IA_TabCycleLeft already mapped in IMC_Grace')

    if 'IA_TabCycleRight' not in existing_actions:
        key_e = unreal.Key()
        key_e.import_text('E')
        mapping_right = unreal.EnhancedActionKeyMapping()
        mapping_right.set_editor_property('action', ia_right)
        mapping_right.set_editor_property('key', key_e)
        mappings.append(mapping_right)
        unreal.log('Mapped E -> IA_TabCycleRight')
    else:
        unreal.log('IA_TabCycleRight already mapped in IMC_Grace')

    imc.set_editor_property('mappings', mappings)
    eal.save_asset('/Game/Core/Input/IMC_Grace')
    unreal.log('IMC_Grace saved')
else:
    unreal.log_error('Failed to load IMC_Grace')

unreal.log('[tab_cycle] Done')
