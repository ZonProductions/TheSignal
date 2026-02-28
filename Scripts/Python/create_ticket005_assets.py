"""
TICKET-005: Create Blueprint assets for player character
Creates: DA_GraceMovement_Default, IA_Move, IA_Look, IA_Sprint, IA_Jump, IA_Interact, IMC_Grace
"""
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

# ── Ensure directories exist ──────────────────────────────────────────
for path in ['/Game/Core/Data', '/Game/Core/Input', '/Game/Core/Input/Actions']:
    if not eal.does_directory_exist(path):
        eal.make_directory(path)

# ── 1. DataAsset: DA_GraceMovement_Default ─────────────────────────
da_path = '/Game/Core/Data/DA_GraceMovement_Default'
if eal.does_asset_exist(da_path):
    unreal.log_warning('DA_GraceMovement_Default already exists, skipping')
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.ZP_GraceMovementConfig)
    da = asset_tools.create_asset('DA_GraceMovement_Default', '/Game/Core/Data', None, factory)
    if da:
        unreal.log('[TICKET-005] Created DA_GraceMovement_Default')
        eal.save_asset(da_path)
    else:
        unreal.log_error('Failed to create DA_GraceMovement_Default')

# ── 2. Input Actions ──────────────────────────────────────────────────
ia_configs = [
    ('IA_Move',     unreal.InputActionValueType.AXIS2D),
    ('IA_Look',     unreal.InputActionValueType.AXIS2D),
    ('IA_Sprint',   unreal.InputActionValueType.BOOLEAN),
    ('IA_Jump',     unreal.InputActionValueType.BOOLEAN),
    ('IA_Interact', unreal.InputActionValueType.BOOLEAN),
]

ia_dir = '/Game/Core/Input/Actions'
for name, value_type in ia_configs:
    ia_path = f'{ia_dir}/{name}'
    if eal.does_asset_exist(ia_path):
        unreal.log_warning(f'{name} already exists, skipping')
        continue
    ia = asset_tools.create_asset(name, ia_dir, unreal.InputAction, None)
    if ia:
        ia.set_editor_property('value_type', value_type)
        unreal.log(f'[TICKET-005] Created {name} (type={value_type})')
        eal.save_asset(ia_path)
    else:
        unreal.log_error(f'Failed to create {name}')

# ── 3. IMC_Grace ──────────────────────────────────────────────────────
imc_path = '/Game/Core/Input/IMC_Grace'
if eal.does_asset_exist(imc_path):
    unreal.log_warning('IMC_Grace already exists, skipping')
else:
    imc = asset_tools.create_asset('IMC_Grace', '/Game/Core/Input', unreal.InputMappingContext, None)
    if imc:
        unreal.log('[TICKET-005] Created IMC_Grace (mappings must be configured in editor)')
        eal.save_asset(imc_path)
    else:
        unreal.log_error('Failed to create IMC_Grace')

unreal.log('[TICKET-005] Asset creation script complete')
