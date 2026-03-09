"""Delete all blind actors across all floors. Re-add as ISMs when layout is locked."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

deleted = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'blind' in label.lower():
        a.destroy_actor()
        deleted += 1

print(f"Deleted {deleted} blind actors")
