"""
Setup IA_Map input action and bind to M key in IMC_Grace.
Also assigns IA_Map to BP_GraceCharacter's MapAction property.
Run via MCP Python endpoint.
"""
import unreal

# --- Step 1: Create IA_Map input action ---
ia_map = unreal.load_asset('/Game/Core/Input/IA_Map')
if ia_map:
    print('IA_Map already exists, skipping creation')
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.InputAction)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    ia_map = asset_tools.create_asset('IA_Map', '/Game/Core/Input', unreal.InputAction, factory)
    if ia_map:
        # Bool type for toggle input
        ia_map.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
        unreal.EditorAssetLibrary.save_asset('/Game/Core/Input/IA_Map')
        print('OK: Created IA_Map (Bool)')
    else:
        print('FAIL: Could not create IA_Map')

if not ia_map:
    raise RuntimeError('IA_Map not available, aborting')

# --- Step 2: Add M key mapping to IMC_Grace ---
imc = unreal.load_asset('/Game/Core/Input/IMC_Grace')
if not imc:
    raise RuntimeError('Could not load IMC_Grace')

mappings = list(imc.get_editor_property('mappings'))
already_mapped = False
for m in mappings:
    action = m.get_editor_property('action')
    if action and action.get_name() == 'IA_Map':
        already_mapped = True
        break

if already_mapped:
    print('IA_Map already mapped in IMC_Grace, skipping')
else:
    new_mapping = unreal.EnhancedActionKeyMapping()
    new_mapping.set_editor_property('action', ia_map)
    m_key = unreal.Key()
    m_key.import_text('M')
    new_mapping.set_editor_property('key', m_key)
    mappings.append(new_mapping)
    imc.set_editor_property('mappings', mappings)
    unreal.EditorAssetLibrary.save_asset('/Game/Core/Input/IMC_Grace')
    print('OK: Added M -> IA_Map mapping to IMC_Grace')

# --- Step 3: Assign IA_Map to BP_GraceCharacter MapAction ---
bp_grace = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
if bp_grace:
    cdo = unreal.get_default_object(bp_grace.generated_class())
    if cdo:
        cdo.set_editor_property('map_action', ia_map)
        unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
        print('OK: Assigned IA_Map to BP_GraceCharacter.MapAction')
    else:
        print('WARN: Could not get CDO for BP_GraceCharacter — assign MapAction manually')
else:
    print('WARN: Could not load BP_GraceCharacter — assign MapAction manually')

print('DONE: Map input setup complete')
