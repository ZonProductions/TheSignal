"""
Add IA_Crouch input action, bind C key in IMC_Grace,
and set CrouchAction on BP_GraceCharacter CDO.
"""
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

ia_dir = '/Game/Core/Input/Actions'
ia_path = f'{ia_dir}/IA_Crouch'

# 1. Create IA_Crouch if not exists
if eal.does_asset_exist(ia_path):
    unreal.log('IA_Crouch already exists, skipping creation')
    ia_crouch = eal.load_asset(ia_path)
else:
    ia_crouch = asset_tools.create_asset('IA_Crouch', ia_dir, unreal.InputAction, None)
    if ia_crouch:
        ia_crouch.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
        eal.save_loaded_asset(ia_crouch, only_if_is_dirty=False)
        unreal.log('[CROUCH] Created IA_Crouch (Boolean)')
    else:
        raise RuntimeError('Failed to create IA_Crouch')

# 2. Add C key mapping to IMC_Grace
imc = eal.load_asset('/Game/Core/Input/IMC_Grace')
mappings = imc.get_editor_property('mappings')

# Check if already bound
already_bound = False
for m in mappings:
    a = m.get_editor_property('action')
    if a and a.get_name() == 'IA_Crouch':
        already_bound = True
        break

if not already_bound:
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', ia_crouch)
    k = unreal.Key()
    k.set_editor_property('key_name', 'LeftControl')
    mapping.set_editor_property('key', k)
    mappings.append(mapping)
    imc.set_editor_property('mappings', mappings)
    eal.save_loaded_asset(imc, only_if_is_dirty=False)
    unreal.log(f'[CROUCH] Added C -> IA_Crouch to IMC_Grace ({len(mappings)} total mappings)')
else:
    unreal.log('[CROUCH] IA_Crouch already in IMC_Grace')

# 3. Set CrouchAction on BP_GraceCharacter CDO
bp = eal.load_asset('/Game/Core/Player/BP_GraceCharacter')
gen_class = bp.generated_class()
cdo = unreal.get_default_object(gen_class)
cdo.set_editor_property('CrouchAction', ia_crouch)
eal.save_loaded_asset(bp, only_if_is_dirty=False)
unreal.log(f'[CROUCH] BP_GraceCharacter CrouchAction = {cdo.get_editor_property("CrouchAction")}')

unreal.log('[CROUCH] Setup complete')
