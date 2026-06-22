import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a and a.get_class().get_name().startswith("ZP_TransitPanel") or (a and "TransitPanel" in a.get_class().get_name()):
        try:
            dests = a.get_editor_property("Destinations")
        except Exception as e:
            print("no Destinations on", a.get_name(), e); continue
        print("=== PANEL:", a.get_actor_label(), "/", a.get_name(), "dest count:", len(dests))
        for d in dests:
            try:
                print("   dest:", d.get_editor_property("DisplayName"),
                      "| LockStyle:", d.get_editor_property("LockStyle"),
                      "| ReqKey:", d.get_editor_property("RequiredKeyItem"),
                      "| ReqObj:", d.get_editor_property("RequiredObjectiveId"))
            except Exception as e:
                print("   dest read err:", e)
