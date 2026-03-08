import unreal

bp = unreal.load_asset('/DialoguePlugin/UI/DemoDialogueWidget')
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

props_to_check = ['Dialogue', 'DialogueAsset', 'dialogue', 'NPCActor', 'DialogueData']
for pname in props_to_check:
    try:
        val = cdo.get_editor_property(pname)
        unreal.log(f'{pname} = {val}')
    except Exception as e:
        unreal.log(f'{pname} = NOT FOUND')

# Also list all UProperty on the class that contain "dialogue" (case insensitive)
unreal.log('--- All properties on DemoDialogueWidget_C ---')
for field in dir(cdo):
    if 'dialogue' in field.lower() or 'dialog' in field.lower():
        unreal.log(f'  FIELD: {field}')
