"""
Create IA_Reload input action, add R key mapping to IMC_Grace,
and set ReloadAction on BP_GraceCharacter CDO.

Pattern follows setup_fire_input.py.
"""
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

IA_DIR = '/Game/Core/Input/Actions'
IA_RELOAD_PATH = f'{IA_DIR}/IA_Reload'
IMC_PATH = '/Game/Core/Input/IMC_Grace'
BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'

# ── 1. Create IA_Reload (Boolean) ─────────────────────────────────
if eal.does_asset_exist(IA_RELOAD_PATH):
    unreal.log('IA_Reload already exists, skipping creation')
    ia_reload = unreal.load_asset(IA_RELOAD_PATH)
else:
    ia_reload = asset_tools.create_asset('IA_Reload', IA_DIR, unreal.InputAction, None)
    if not ia_reload:
        raise RuntimeError('Failed to create IA_Reload')
    ia_reload.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
    eal.save_asset(IA_RELOAD_PATH)
    unreal.log('Created IA_Reload (Boolean)')

# ── 2. Add R key -> IA_Reload to IMC_Grace ─────────────────────────
imc = unreal.load_asset(IMC_PATH)
if not imc:
    raise RuntimeError(f'Failed to load {IMC_PATH}')

mappings = imc.get_editor_property('mappings')

already_bound = False
for m in mappings:
    a = m.get_editor_property('action')
    if a and a.get_name() == 'IA_Reload':
        already_bound = True
        break

if already_bound:
    unreal.log('IA_Reload already bound in IMC_Grace, skipping')
else:
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', ia_reload)

    k = unreal.Key()
    k.set_editor_property('key_name', 'R')
    mapping.set_editor_property('key', k)

    mappings.append(mapping)
    imc.set_editor_property('mappings', mappings)
    eal.save_loaded_asset(imc, only_if_is_dirty=False)
    unreal.log(f'Added R -> IA_Reload to IMC_Grace ({len(mappings)} total mappings)')

# ── 3. Set ReloadAction on BP_GraceCharacter CDO ──────────────────
bp_grace = unreal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)
cdo.set_editor_property('ReloadAction', ia_reload)
eal.save_asset(BP_GRACE_PATH)
unreal.log(f'BP_GraceCharacter ReloadAction set to: {cdo.get_editor_property("ReloadAction")}')

unreal.log('Reload input setup complete — R -> IA_Reload -> ReloadAction')
