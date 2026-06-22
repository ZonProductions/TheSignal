import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
changed_any = False
for a in eas.get_all_level_actors():
    if a and "TransitPanel" in a.get_class().get_name():
        dests = a.get_editor_property("Destinations")
        changed = False
        for i in range(len(dests)):
            d = dests[i]
            if d.get_editor_property("LockStyle") != unreal.ZP_TransitLockStyle.HIDDEN_UNTIL_KNOWN:
                d.set_editor_property("LockStyle", unreal.ZP_TransitLockStyle.HIDDEN_UNTIL_KNOWN)
                dests[i] = d
                changed = True
        if changed:
            a.set_editor_property("Destinations", dests)
            changed_any = True
            print("SET HiddenUntilKnown on", a.get_actor_label())
        # verify
        for d in a.get_editor_property("Destinations"):
            print("   now:", d.get_editor_property("DisplayName"), "->", d.get_editor_property("LockStyle"))
if changed_any:
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.save_current_level()
    print("SAVED level")
else:
    print("nothing changed")
