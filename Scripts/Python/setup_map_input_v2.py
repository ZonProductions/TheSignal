"""
Setup IA_Map input action, bind M key, assign to BP_GraceCharacter.
Also replaces MapPickup with correct AreaID.
Run via MCP Python endpoint.
"""
import unreal

# --- Step 1: Create IA_Map input action ---
ia_map = unreal.load_asset('/Game/Core/Input/IA_Map')
if not ia_map:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.InputAction)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    ia_map = asset_tools.create_asset('IA_Map', '/Game/Core/Input', unreal.InputAction, factory)
    if ia_map:
        ia_map.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
        print('Created IA_Map')
    else:
        raise RuntimeError('Failed to create IA_Map')

# Force save
unreal.EditorAssetLibrary.save_asset('/Game/Core/Input/IA_Map', only_if_is_dirty=False)
print('Saved IA_Map')

# Verify it loads back
ia_check = unreal.load_asset('/Game/Core/Input/IA_Map')
print('Verify IA_Map load:', ia_check)

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

if not already_mapped:
    new_mapping = unreal.EnhancedActionKeyMapping()
    new_mapping.set_editor_property('action', ia_map)
    m_key = unreal.Key()
    m_key.import_text('M')
    new_mapping.set_editor_property('key', m_key)
    mappings.append(new_mapping)
    imc.set_editor_property('mappings', mappings)
    unreal.EditorAssetLibrary.save_asset('/Game/Core/Input/IMC_Grace', only_if_is_dirty=False)
    print('Added M -> IA_Map to IMC_Grace and saved')
else:
    print('IA_Map already in IMC_Grace')

# --- Step 3: Assign IA_Map to BP_GraceCharacter ---
bp_grace = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
if bp_grace:
    cdo = unreal.get_default_object(bp_grace.generated_class())
    if cdo:
        cdo.set_editor_property('map_action', ia_map)
        unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter', only_if_is_dirty=False)
        # Verify
        check = cdo.get_editor_property('map_action')
        print('MapAction set and saved. Verify:', check)
    else:
        print('FAIL: No CDO')
else:
    print('FAIL: Could not load BP_GraceCharacter')

# --- Step 4: MapPickup ---
# NOTE: Do NOT spawn MapPickup from Python — FName properties don't persist
# through level serialization when set via set_editor_property.
# Place manually: Place Actors → search "MapPickup" → set AreaID in Details panel.
print('NOTE: Place MapPickup manually in editor (Python FName serialization unreliable)')

print('DONE')
