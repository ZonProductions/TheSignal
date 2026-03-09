"""Push all glass panels into the wall for flush fit."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
moved = 0
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if 'line_box_glass3' in label or 'line_box_glass4' in label:
        loc = a.get_actor_location()
        new_y = loc.y - 5.0
        a.set_actor_location(unreal.Vector(loc.x, new_y, loc.z), False, False)
        print(f"  {label}: Y {loc.y:.1f} -> {new_y:.1f}")
        moved += 1

print(f"\nMoved {moved} panels 5 UU into wall")
