import unreal

bp = unreal.load_asset('/DialoguePlugin/UI/DemoDialogueWidget')
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

# Check ALL properties on CDO
unreal.log('=== ALL PROPERTIES ===')
for field in sorted(dir(cdo)):
    if field.startswith('_'):
        continue
    if callable(getattr(cdo, field, None)):
        continue
    try:
        val = cdo.get_editor_property(field)
        unreal.log(f'  {field} = {val}')
    except:
        pass

# Check parent class chain
unreal.log('=== CLASS HIERARCHY ===')
cls = gen
while cls:
    unreal.log(f'  {cls.get_name()}')
    cls = cls.get_super_class() if hasattr(cls, 'get_super_class') else None

# Check for functions containing "dialogue" or "initialize"
unreal.log('=== METHODS (dialogue/init related) ===')
for field in sorted(dir(cdo)):
    if any(kw in field.lower() for kw in ['dialogue', 'dialog', 'init', 'start', 'begin', 'setup']):
        unreal.log(f'  {field} (callable={callable(getattr(cdo, field, None))})')
