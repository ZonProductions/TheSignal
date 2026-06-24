import unreal
ES = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
panels = [a for a in ES.get_all_level_actors() if a.get_class().get_name().startswith("ZP_TransitPanel")
          or "TransitPanel" in a.get_class().get_name()]
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
print("LEVEL:", world.get_name() if world else "?", " transit panels:", len(panels))

changed = False
for p in panels:
    try:
        dests = p.get_editor_property("Destinations")
    except Exception as e:
        print("  ", p.get_actor_label(), "no Destinations:", e); continue
    print("PANEL", p.get_actor_label(), "dests:", len(dests))
    for i, d in enumerate(dests):
        did = d.get_editor_property("DestinationId")
        disp = d.get_editor_property("DisplayName")
        robj = d.get_editor_property("RequiredObjectiveId")
        rflag = d.get_editor_property("RequiredFlag")
        rkey = d.get_editor_property("RequiredKeyItem")
        print("   [%d] id=%s disp='%s' reqObj=%s reqFlag=%s reqKey=%s"
              % (i, did, str(disp), robj, rflag, rkey))
        blob = (str(did) + " " + str(disp)).lower()
        if "empty" in blob:
            d.set_editor_property("RequiredFlag", "empty_floor_available")
            dests[i] = d
            changed = True
            print("       -> SET RequiredFlag = empty_floor_available")
    if changed:
        p.set_editor_property("Destinations", dests)

if changed:
    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("Saved level:", saved)
else:
    print("No 'empty' destination found in this level's transit panels — set RequiredFlag by hand.")
print("DONE")
