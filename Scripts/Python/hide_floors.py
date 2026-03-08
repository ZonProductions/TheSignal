"""Hide/show floors by prefix. Hides floors 3-5, keeps 1-2 visible for editing.
Adjust VISIBLE_FLOORS to control which floors are shown.
"""
import unreal

VISIBLE_FLOORS = {1, 2}  # Change this to show different floors

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

hidden = 0
shown = 0

for a in all_actors:
    label = a.get_actor_label()

    if label.startswith('F2_'):
        floor = 2
    elif label.startswith('F3_'):
        floor = 3
    elif label.startswith('F4_'):
        floor = 4
    elif label.startswith('F5_'):
        floor = 5
    else:
        floor = 1

    should_show = floor in VISIBLE_FLOORS
    a.set_is_temporarily_hidden_in_editor(not should_show)

    if should_show:
        shown += 1
    else:
        hidden += 1

print(f"Visible floors: {sorted(VISIBLE_FLOORS)}")
print(f"Shown: {shown} actors, Hidden: {hidden} actors")
