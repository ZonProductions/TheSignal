import unreal

bp = unreal.load_asset('/Game/Dialogue/WBP_DialogueWidget')
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

# Check all property names that might be dialogue-related
for field in sorted(dir(cdo)):
    if 'dialogue' in field.lower() or 'dialog' in field.lower() or field.startswith('in_'):
        try:
            val = cdo.get_editor_property(field)
            unreal.log(f'  {field} = {val}')
        except Exception as e:
            unreal.log(f'  {field} ERROR: {e}')

# Specifically check InDialogue vs in_dialogue
for name in ['InDialogue', 'in_dialogue', 'Indialogue', 'inDialogue']:
    try:
        val = cdo.get_editor_property(name)
        unreal.log(f'  FOUND: {name} = {val}')
    except:
        pass

# Also check via UE reflection (FindPropertyByName)
unreal.log('\n--- C++ property lookup test ---')
# Simulate what our C++ code does: FindPropertyByName("InDialogue")
# We can check by listing all FProperty names on the class
unreal.log(f'Class path: {gen.get_path_name()}')
