"""
Configure IMC_Grace with key mappings.
WASD → IA_Move (Axis2D), Mouse XY → IA_Look (Axis2D),
Shift → IA_Sprint, Space → IA_Jump, E → IA_Interact

Modifiers must be created via unreal.new_object() with outer=imc
so they persist as sub-objects when the asset is saved.
"""
import unreal

eal = unreal.EditorAssetLibrary

# Load assets
imc = eal.load_asset('/Game/Core/Input/IMC_Grace')
ia_move = eal.load_asset('/Game/Core/Input/Actions/IA_Move')
ia_look = eal.load_asset('/Game/Core/Input/Actions/IA_Look')
ia_sprint = eal.load_asset('/Game/Core/Input/Actions/IA_Sprint')
ia_jump = eal.load_asset('/Game/Core/Input/Actions/IA_Jump')
ia_interact = eal.load_asset('/Game/Core/Input/Actions/IA_Interact')

if not all([imc, ia_move, ia_look, ia_sprint, ia_jump, ia_interact]):
    unreal.log_error('Failed to load one or more assets!')
    raise RuntimeError('Asset load failed')

# Clear existing mappings
mappings = imc.get_editor_property('mappings')
while len(mappings) > 0:
    mappings.pop()

def make_key(key_name):
    k = unreal.Key()
    k.set_editor_property('key_name', key_name)
    return k

def make_negate():
    """Create Negate modifier as sub-object of IMC (persists on save)."""
    return unreal.new_object(type=unreal.InputModifierNegate, outer=imc)

def make_swizzle_yxz():
    """Create Swizzle YXZ modifier as sub-object of IMC (persists on save)."""
    mod = unreal.new_object(type=unreal.InputModifierSwizzleAxis, outer=imc)
    mod.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
    return mod

def add_mapping(action, key_name, modifiers=None):
    mapping = unreal.EnhancedActionKeyMapping()
    mapping.set_editor_property('action', action)
    mapping.set_editor_property('key', make_key(key_name))
    if modifiers:
        mod_array = mapping.get_editor_property('modifiers')
        for mod in modifiers:
            mod_array.append(mod)
    mappings.append(mapping)

# ── IA_Move (WASD → Axis2D) ───────────────────────────────────────
# Digital key press → raw (1,0,0). Swizzle YXZ → (0,1,0) = Y axis.
add_mapping(ia_move, 'W', [make_swizzle_yxz()])
add_mapping(ia_move, 'S', [make_swizzle_yxz(), make_negate()])
add_mapping(ia_move, 'A', [make_negate()])
add_mapping(ia_move, 'D')

# ── IA_Look (Mouse → Axis2D) ─────────────────────────────────────
add_mapping(ia_look, 'Mouse2D')

# ── IA_Sprint (Left Shift) ───────────────────────────────────────
add_mapping(ia_sprint, 'LeftShift')

# ── IA_Jump (Space) ──────────────────────────────────────────────
add_mapping(ia_jump, 'SpaceBar')

# ── IA_Interact (E) ──────────────────────────────────────────────
add_mapping(ia_interact, 'E')

# Write back and save
imc.set_editor_property('mappings', mappings)
eal.save_loaded_asset(imc, only_if_is_dirty=False)

unreal.log(f'[TICKET-005] IMC_Grace configured with {len(mappings)} mappings')
for i, m in enumerate(mappings):
    a = m.get_editor_property('action')
    action_name = a.get_name() if a else 'None'
    key_name = str(m.get_editor_property('key').get_editor_property('key_name'))
    mods = m.get_editor_property('modifiers')
    mod_details = []
    if mods:
        for mod in mods:
            mod_type = type(mod).__name__
            try:
                order = mod.get_editor_property('order')
                mod_details.append(f'{mod_type}(order={order})')
            except:
                mod_details.append(mod_type)
    unreal.log(f'  [{i}] {action_name} <- {key_name} | mods: {mod_details}')
