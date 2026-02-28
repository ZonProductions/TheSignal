"""
K001: Create IA_Fire input action, add LeftMouseButton mapping to IMC_Grace,
and set FireAction on BP_GraceCharacter CDO.

Pattern follows setup_aim_input.py and add_crouch_input.py.
"""
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

IA_DIR = '/Game/Core/Input/Actions'
IA_FIRE_PATH = f'{IA_DIR}/IA_Fire'
IMC_PATH = '/Game/Core/Input/IMC_Grace'
BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'

# ── 1. Create IA_Fire (Boolean) ──────────────────────────────────
if eal.does_asset_exist(IA_FIRE_PATH):
    unreal.log('IA_Fire already exists, skipping creation')
    ia_fire = eal.load_asset(IA_FIRE_PATH)
else:
    ia_fire = asset_tools.create_asset('IA_Fire', IA_DIR, unreal.InputAction, None)
    if not ia_fire:
        raise RuntimeError('Failed to create IA_Fire')
    ia_fire.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(IA_FIRE_PATH)
    unreal.log('Created IA_Fire (Boolean)')

# ── 2. Add LeftMouseButton -> IA_Fire to IMC_Grace ───────────────
imc = eal.load_asset(IMC_PATH)
if not imc:
    raise RuntimeError(f'Failed to load {IMC_PATH}')

mappings = imc.get_editor_property('mappings')

already_bound = False
for m in mappings:
    a = m.get_editor_property('action')
    if a and a.get_name() == 'IA_Fire':
        already_bound = True
        break

if already_bound:
    unreal.log('IA_Fire already bound in IMC_Grace, skipping')
else:
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', ia_fire)

    k = unreal.Key()
    k.set_editor_property('key_name', 'LeftMouseButton')
    mapping.set_editor_property('key', k)

    mappings.append(mapping)
    imc.set_editor_property('mappings', mappings)
    eal.save_loaded_asset(imc, only_if_is_dirty=False)
    unreal.log(f'Added LeftMouseButton -> IA_Fire to IMC_Grace ({len(mappings)} total mappings)')

# ── 3. Set FireAction on BP_GraceCharacter CDO ───────────────────
bp_grace = eal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)
cdo.set_editor_property('FireAction', ia_fire)
eal.save_asset(BP_GRACE_PATH)
unreal.log(f'[K001] BP_GraceCharacter FireAction set to: {cdo.get_editor_property("FireAction")}')

unreal.log('[K001] Fire input setup complete — LMB -> IA_Fire -> FireAction')
