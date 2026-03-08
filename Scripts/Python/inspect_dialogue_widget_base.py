import unreal

# Find the DialogueUserWidget C++ base class
try:
    cls = unreal.find_class('DialogueUserWidget')
    unreal.log(f'DialogueUserWidget class: {cls}')

    # Get CDO and check properties
    cdo = unreal.get_default_object(cls)
    unreal.log('=== DialogueUserWidget properties ===')
    for field in sorted(dir(cdo)):
        if field.startswith('_'):
            continue
        if 'dialogue' in field.lower() or 'npc' in field.lower() or 'actor' in field.lower():
            try:
                val = cdo.get_editor_property(field)
                unreal.log(f'  {field} = {val}')
            except Exception as e:
                is_callable = callable(getattr(cdo, field, None))
                unreal.log(f'  {field} (callable={is_callable})')
except Exception as e:
    unreal.log(f'DialogueUserWidget not found: {e}')

# Also check if there's a dialogue interface
try:
    iface = unreal.find_class('DialogueInterface')
    unreal.log(f'DialogueInterface: {iface}')
except:
    unreal.log('DialogueInterface: NOT FOUND')
