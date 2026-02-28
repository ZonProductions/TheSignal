"""
Create IA_Aim input action, add RightMouseButton mapping to IMC_Grace,
and set AimAction on BP_GraceCharacter CDO.
"""
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

IA_DIR = '/Game/Core/Input/Actions'
IA_AIM_PATH = f'{IA_DIR}/IA_Aim'
IMC_PATH = '/Game/Core/Input/IMC_Grace'
BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'

# ── 1. Create IA_Aim (Boolean) ────────────────────────────────────
if eal.does_asset_exist(IA_AIM_PATH):
    unreal.log('IA_Aim already exists, skipping creation')
    ia_aim = eal.load_asset(IA_AIM_PATH)
else:
    ia_aim = asset_tools.create_asset('IA_Aim', IA_DIR, unreal.InputAction, None)
    if not ia_aim:
        raise RuntimeError('Failed to create IA_Aim')
    ia_aim.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(IA_AIM_PATH)
    unreal.log('Created IA_Aim (Boolean)')

# ── 2. Add RightMouseButton -> IA_Aim to IMC_Grace ────────────────
imc = eal.load_asset(IMC_PATH)
if not imc:
    raise RuntimeError(f'Failed to load {IMC_PATH}')

mappings = imc.get_editor_property('mappings')

# Check if already bound
already_bound = False
for m in mappings:
    a = m.get_editor_property('action')
    if a and a.get_name() == 'IA_Aim':
        already_bound = True
        break

if already_bound:
    unreal.log('IA_Aim already bound in IMC_Grace, skipping')
else:
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', ia_aim)

    k = unreal.Key()
    k.set_editor_property('key_name', 'RightMouseButton')
    mapping.set_editor_property('key', k)

    mappings.append(mapping)
    imc.set_editor_property('mappings', mappings)
    eal.save_loaded_asset(imc, only_if_is_dirty=False)
    unreal.log('Added RightMouseButton -> IA_Aim to IMC_Grace')

# ── 3. Set AimAction on BP_GraceCharacter CDO ─────────────────────
bp_grace = eal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)
cdo.set_editor_property('AimAction', ia_aim)
eal.save_asset(BP_GRACE_PATH)
unreal.log(f'BP_GraceCharacter AimAction set to: {cdo.get_editor_property("AimAction")}')

unreal.log('[PEEK] All aim input setup complete')
