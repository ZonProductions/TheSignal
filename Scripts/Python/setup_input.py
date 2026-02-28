"""
Setup Enhanced Input: IMC_Grace mapping context with all key bindings.
Run inside UE5 Python console or via BlueprintMCP /api/python endpoint.
"""
import unreal

el = unreal.EditorAssetLibrary

# Load assets
imc = el.load_asset('/Game/Player/Input/IMC_Grace')
ia_move = el.load_asset('/Game/Player/Input/IA_Move')
ia_look = el.load_asset('/Game/Player/Input/IA_Look')
ia_sprint = el.load_asset('/Game/Player/Input/IA_Sprint')
ia_interact = el.load_asset('/Game/Player/Input/IA_Interact')

# Clear any existing mappings
imc.unmap_all()

def make_key(name):
    k = unreal.Key()
    k.import_text(name)
    return k

# WASD movement
imc.map_key(ia_move, make_key('W'))
imc.map_key(ia_move, make_key('S'))
imc.map_key(ia_move, make_key('A'))
imc.map_key(ia_move, make_key('D'))

# Mouse look
imc.map_key(ia_look, make_key('Mouse2D'))

# Sprint (left shift)
imc.map_key(ia_sprint, make_key('LeftShift'))

# Interact
imc.map_key(ia_interact, make_key('E'))

# Now add modifiers to WASD for proper axis mapping
# W = +Y, S = -Y, A = -X, D = +X
mappings = imc.get_editor_property('mappings')
print(f'Total mappings: {len(mappings)}')

for i, m in enumerate(mappings):
    key_name = m.get_editor_property('key').export_text()
    action_name = str(m.get_editor_property('action'))
    print(f'  [{i}] {key_name} -> {action_name}')

# Add Swizzle and Negate modifiers for WASD
# W (+Y): Swizzle YXZ (converts X axis input to Y)
# S (-Y): Swizzle YXZ + Negate
# A (-X): Negate
# D (+X): no modifiers needed

# W: Swizzle input axis YXZ
swizzle_w = unreal.InputModifierSwizzleAxis()
swizzle_w.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
mappings[0].set_editor_property('modifiers', [swizzle_w])

# S: Swizzle YXZ + Negate
swizzle_s = unreal.InputModifierSwizzleAxis()
swizzle_s.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
negate_s = unreal.InputModifierNegate()
mappings[1].set_editor_property('modifiers', [swizzle_s, negate_s])

# A: Negate
negate_a = unreal.InputModifierNegate()
mappings[2].set_editor_property('modifiers', [negate_a])

# D: no modifiers (raw +X)

el.save_asset('/Game/Player/Input/IMC_Grace')
print('IMC_Grace configured successfully')
