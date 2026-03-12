"""Verify WBP_Notes complete state."""
import unreal

bp = unreal.load_asset('/Game/EasyGameUI/EasyOptionsMenu/Core/WBP_Notes')
gc = bp.generated_class()
cdo = unreal.get_default_object(gc)

# Check methods that prove parent = ZP_NotesWidget
has_bind = hasattr(cdo, 'bind_to_note_component')
has_refresh = hasattr(cdo, 'refresh_note_list')
has_select = hasattr(cdo, 'select_note')
print(f'Parent is ZP_NotesWidget: {has_bind and has_refresh and has_select}')

# Check NoteEntryWidgetClass default
try:
    val = cdo.get_editor_property('note_entry_widget_class')
    print(f'NoteEntryWidgetClass: {val.get_name() if val else None}')
except:
    print('NoteEntryWidgetClass: NOT accessible')

# Check widget tree
tree = bp.get_editor_property('widget_tree')
if tree:
    root = tree.root_widget
    print(f'Root widget: {root.get_name()} ({root.get_class().get_name()})' if root else 'Root: None')

    # Walk children
    def print_tree(widget, indent=0):
        prefix = '  ' * indent
        name = widget.get_name()
        cls = widget.get_class().get_name()
        print(f'{prefix}{name} ({cls})')
        if hasattr(widget, 'get_child_count'):
            for i in range(widget.get_child_count()):
                child = widget.get_child_at(i)
                if child:
                    print_tree(child, indent + 1)

    if root:
        print_tree(root)
else:
    print('Widget tree: None')
