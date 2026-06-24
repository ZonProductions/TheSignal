import unreal
ES = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = ES.get_all_level_actors()

def near(loc, items, r=1500):
    out = []
    for a in items:
        d = (a.get_actor_location() - loc).size()
        if d <= r:
            out.append((d, a))
    out.sort(key=lambda x: x[0])
    return out

readers = [a for a in actors if a.get_class().get_name().startswith("BP_CardReaderPanel") or a.get_class().get_name().startswith("ZP_CardReaderPanel")]
doors    = [a for a in actors if "Door" in a.get_class().get_name()]
containers = [a for a in actors if "Container" in a.get_class().get_name() or "Container" in a.get_actor_label()]
notes    = [a for a in actors if "Note" in a.get_actor_label() or "Mysterious" in a.get_actor_label()]

print("DOORS total:", len(doors), "CONTAINERS:", len(containers), "NOTE-ish:", len(notes))
for c in containers:
    l = c.get_actor_location(); print("  CONTAINER", c.get_actor_label(), c.get_class().get_name(), "@(%.0f,%.0f,%.0f)" % (l.x,l.y,l.z))
for n in notes:
    l = n.get_actor_location(); print("  NOTE", n.get_actor_label(), n.get_class().get_name(), "@(%.0f,%.0f,%.0f)" % (l.x,l.y,l.z))

for r in readers:
    loc = r.get_actor_location()
    print("=== %s @(%.0f,%.0f,%.0f) ===" % (r.get_actor_label(), loc.x, loc.y, loc.z))
    try:
        ld = r.get_editor_property("LinkedDoor")
        if ld:
            dl = ld.get_actor_location(); print("  LinkedDoor:", ld.get_actor_label(), "@(%.0f,%.0f,%.0f)" % (dl.x,dl.y,dl.z))
        else:
            print("  LinkedDoor: None")
    except Exception as e:
        print("  LinkedDoor ERR", e)
    print("  nearest doors:")
    for d, a in near(loc, doors, 800)[:3]:
        print("     %.0f  %s (%s)" % (d, a.get_actor_label(), a.get_class().get_name()))
    print("  nearest containers/notes:")
    for d, a in near(loc, containers + notes, 4000)[:3]:
        print("     %.0f  %s (%s)" % (d, a.get_actor_label(), a.get_class().get_name()))
