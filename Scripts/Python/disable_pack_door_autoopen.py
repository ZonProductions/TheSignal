"""Disable auto-open behavior on all pack door BPs.

Pack doors (BigCompany) have built-in overlap triggers + timelines that auto-open.
We disable their BoxComponent collision so the pack logic never fires.
Our AZP_InteractDoor handles E-key interaction instead.
"""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Find all pack door BPs (they have Timeline components = pack auto-open logic)
pack_doors = []
for a in all_actors:
    name = a.get_actor_label()
    class_name = a.get_class().get_name()
    # Pack BPs end in _C and have Timeline components
    if class_name.endswith('_C') and 'door' in name.lower():
        timelines = a.get_components_by_class(unreal.TimelineComponent)
        if timelines and len(timelines) > 0:
            pack_doors.append(a)

print(f"Found {len(pack_doors)} pack door BPs with auto-open logic:")

for a in pack_doors:
    name = a.get_actor_label()
    print(f"\n=== {name} ({a.get_class().get_name()}) ===")

    # List all components
    comps = a.get_components_by_class(unreal.ActorComponent)
    for c in comps:
        print(f"  {c.get_name()} ({c.get_class().get_name()})")

    # Disable BoxComponent overlap (the auto-open trigger)
    boxes = a.get_components_by_class(unreal.BoxComponent)
    for box in boxes:
        box.set_editor_property('generate_overlap_events', False)
        box.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        print(f"  -> Disabled overlap on {box.get_name()}")

    # Also disable actor tick so timeline doesn't play
    a.set_actor_tick_enabled(False)
    print(f"  -> Disabled actor tick")

print(f"\nDisabled auto-open on {len(pack_doors)} pack door BPs")
