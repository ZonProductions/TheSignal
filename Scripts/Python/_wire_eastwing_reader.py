import unreal
ES = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = ES.get_all_level_actors()
readers = [a for a in actors if a.get_class().get_name().startswith("BP_CardReaderPanel")
           or a.get_class().get_name().startswith("ZP_CardReaderPanel")]
if not readers:
    print("NO READERS FOUND")
else:
    # "East wing" = the reader at the greatest +Y (UE convention: +Y is East).
    east = max(readers, key=lambda a: a.get_actor_location().y)
    east.set_editor_property("ObjectiveFlagOnTry", "tried_card_reader")
    east.set_editor_property("ObjectiveFlagOnUnlock", "east_wing_unlocked")
    loc = east.get_actor_location()
    print("WIRED east-wing reader: %s @(%.0f,%.0f,%.0f)" % (east.get_actor_label(), loc.x, loc.y, loc.z))
    print("  ObjectiveFlagOnTry   =", east.get_editor_property("ObjectiveFlagOnTry"))
    print("  ObjectiveFlagOnUnlock=", east.get_editor_property("ObjectiveFlagOnUnlock"))
    for a in readers:
        if a is not east:
            l = a.get_actor_location()
            print("  (left untouched: %s @(%.0f,%.0f,%.0f))" % (a.get_actor_label(), l.x, l.y, l.z))
    # Save the level so the wiring persists.
    LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    saved = LES.save_current_level()
    print("save_current_level ->", saved)
