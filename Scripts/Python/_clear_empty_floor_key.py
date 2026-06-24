import unreal
ES = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
panels = [a for a in ES.get_all_level_actors() if "TransitPanel" in a.get_class().get_name()]
changed = False
for p in panels:
    try:
        dests = p.get_editor_property("Destinations")
    except Exception:
        continue
    for i, d in enumerate(dests):
        blob = (str(d.get_editor_property("DestinationId")) + " " + str(d.get_editor_property("DisplayName"))).lower()
        if "empty" in blob:
            d.set_editor_property("RequiredFlag", "empty_floor_available")  # ensure flag gate
            d.set_editor_property("RequiredKeyItem", None)                  # drop the non-persistent note gate
            dests[i] = d
            changed = True
            print("Empty-floor dest: RequiredFlag=empty_floor_available, RequiredKeyItem cleared.")
            print("  verify reqFlag=", d.get_editor_property("RequiredFlag"),
                  " reqKey=", d.get_editor_property("RequiredKeyItem"))
    if changed:
        p.set_editor_property("Destinations", dests)
if changed:
    print("Saved:", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
else:
    print("No 'empty' destination found.")
print("DONE")
