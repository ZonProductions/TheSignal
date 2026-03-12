"""Check WBP_Notes parent class chain."""
import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
gc = bp.generated_class()
cdo = unreal.get_default_object(gc)

# Walk up class hierarchy
cls = cdo.get_class()
print(f'CDO class: {cls.get_name()}')

# Check all properties for anything note-related
props = [p for p in dir(cdo) if not p.startswith('_')]
note_props = [p for p in props if 'note' in p.lower() or 'entry' in p.lower() or 'scroll' in p.lower()]
print(f'Note/entry/scroll props: {note_props}')

# Check if it's a ZP_NotesWidget by looking for the C++ class
zp_notes_class = unreal.find_class('ZP_NotesWidget')
print(f'ZP_NotesWidget class found: {zp_notes_class}')

# Check isinstance
if zp_notes_class:
    is_child = gc.is_child_of(zp_notes_class) if hasattr(gc, 'is_child_of') else 'unknown'
    print(f'WBP_Notes_C is child of ZP_NotesWidget: {is_child}')

# Try to get parent class name from BP object directly
print(f'BP type: {type(bp)}')
# Check all BP properties
bp_props = [p for p in dir(bp) if 'parent' in p.lower() or 'class' in p.lower()]
print(f'BP parent/class props: {bp_props}')
