import unreal

imc = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IMC_TacticalShooterPack')
if not imc:
    print("ERROR: IMC not found")
else:
    mappings = imc.get_editor_property('mappings')
    print(f"IMC has {len(mappings)} mappings:")
    for i, m in enumerate(mappings):
        action = m.get_editor_property('action')
        action_name = action.get_name() if action else 'None'
        key = m.get_editor_property('key')
        triggers = m.get_editor_property('triggers')
        modifiers = m.get_editor_property('modifiers')
        trigger_str = ', '.join([type(t).__name__ for t in triggers]) if triggers else 'default'
        mod_str = ', '.join([type(mod).__name__ for mod in modifiers]) if modifiers else 'none'
        print(f"  [{i}] {action_name} -> {key} | triggers: {trigger_str} | mods: {mod_str}")
