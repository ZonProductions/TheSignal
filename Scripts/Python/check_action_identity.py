import unreal

# Load the IMC and get its IA_Move reference
imc = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IMC_TacticalShooterPack')

# Load IA_Move directly by path (same way C++ ConstructorHelpers does)
ia_move_direct = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IA_Move')
ia_look_direct = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IA_Look')
ia_jump_direct = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IA_Jump')

# Get the IA_Move from IMC mappings
imc_move = None
imc_look = None
imc_jump = None
for m in imc.get_editor_property('mappings'):
    action = m.get_editor_property('action')
    if action:
        name = action.get_name()
        if name == 'IA_Move' and imc_move is None:
            imc_move = action
        elif name == 'IA_Look' and imc_look is None:
            imc_look = action
        elif name == 'IA_Jump' and imc_jump is None:
            imc_jump = action

# Compare object identity
print(f"IA_Move - Direct: {ia_move_direct}, IMC: {imc_move}, Same: {ia_move_direct == imc_move}")
print(f"IA_Look - Direct: {ia_look_direct}, IMC: {imc_look}, Same: {ia_look_direct == imc_look}")
print(f"IA_Jump - Direct: {ia_jump_direct}, IMC: {imc_jump}, Same: {ia_jump_direct == imc_jump}")

# Check action value types
print(f"IA_Move ValueType: {ia_move_direct.get_editor_property('value_type')}")
print(f"IA_Look ValueType: {ia_look_direct.get_editor_property('value_type')}")
print(f"IA_Jump ValueType: {ia_jump_direct.get_editor_property('value_type')}")

# Check consume input
try:
    print(f"IA_Move consume: {ia_move_direct.get_editor_property('consume_input')}")
    print(f"IA_Jump consume: {ia_jump_direct.get_editor_property('consume_input')}")
except:
    print("consume_input property not found")

# Check triggers on actions themselves
try:
    move_triggers = ia_move_direct.get_editor_property('triggers')
    jump_triggers = ia_jump_direct.get_editor_property('triggers')
    print(f"IA_Move triggers: {[type(t).__name__ for t in move_triggers] if move_triggers else 'none (implicit)'}")
    print(f"IA_Jump triggers: {[type(t).__name__ for t in jump_triggers] if jump_triggers else 'none (implicit)'}")
except Exception as e:
    print(f"Error reading triggers: {e}")
