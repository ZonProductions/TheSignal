import unreal

# Check actual key names in IMC
imc = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Inputs/IMC_TacticalShooterPack')
if imc:
    mappings = imc.get_editor_property('mappings')
    for i, m in enumerate(mappings):
        action = m.get_editor_property('action')
        action_name = action.get_name() if action else 'None'
        key = m.get_editor_property('key')
        # Try to get key name via to_string or key_name
        try:
            key_name = key.get_editor_property('key_name')
        except:
            key_name = str(key)
        print(f"  [{i}] {action_name} -> key: {key_name}")

# Check active mapping contexts on player
print("--- Active IMCs on player ---")
pc = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
# Try alternative: get player controller from PIE
try:
    world = unreal.EditorLevelLibrary.get_game_world()
    print(f"World: {world}")
except:
    print("Could not get game world (PIE may not be running)")
