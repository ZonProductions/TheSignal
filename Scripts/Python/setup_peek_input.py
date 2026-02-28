"""
Create IA_Peek input action, add Q key mapping to IMC_Grace,
and set PeekAction on BP_GraceCharacter CDO.

Pattern follows setup_reload_input.py.
"""
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

IA_DIR = '/Game/Core/Input/Actions'
IA_PEEK_PATH = f'{IA_DIR}/IA_Peek'
IMC_PATH = '/Game/Core/Input/IMC_Grace'
BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'

# -- 1. Create IA_Peek (Boolean) --
if eal.does_asset_exist(IA_PEEK_PATH):
    unreal.log('IA_Peek already exists, skipping creation')
    ia_peek = unreal.load_asset(IA_PEEK_PATH)
else:
    ia_peek = asset_tools.create_asset('IA_Peek', IA_DIR, unreal.InputAction, None)
    if not ia_peek:
        raise RuntimeError('Failed to create IA_Peek')
    ia_peek.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(IA_PEEK_PATH)
    unreal.log('Created IA_Peek (Boolean)')

# -- 2. Add Q key -> IA_Peek to IMC_Grace --
imc = unreal.load_asset(IMC_PATH)
if not imc:
    raise RuntimeError(f'Failed to load {IMC_PATH}')

mappings = imc.get_editor_property('mappings')

already_bound = False
for m in mappings:
    a = m.get_editor_property('action')
    if a and a.get_name() == 'IA_Peek':
        already_bound = True
        break

if already_bound:
    unreal.log('IA_Peek already bound in IMC_Grace, skipping')
else:
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', ia_peek)

    k = unreal.Key()
    k.set_editor_property('key_name', 'Q')
    mapping.set_editor_property('key', k)

    mappings.append(mapping)
    imc.set_editor_property('mappings', mappings)
    eal.save_loaded_asset(imc, only_if_is_dirty=False)
    unreal.log(f'Added Q -> IA_Peek to IMC_Grace ({len(mappings)} total mappings)')

# -- 3. Set PeekAction on BP_GraceCharacter CDO --
bp_grace = unreal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)
cdo.set_editor_property('PeekAction', ia_peek)
eal.save_asset(BP_GRACE_PATH)
unreal.log(f'BP_GraceCharacter PeekAction set to: {cdo.get_editor_property("PeekAction")}')

unreal.log('Peek input setup complete — Q -> IA_Peek -> PeekAction')
