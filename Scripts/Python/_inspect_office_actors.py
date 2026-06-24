import unreal
EAS = unreal.EditorActorSubsystem()
actors = EAS.get_all_level_actors()
world = unreal.EditorLevelLibrary.get_editor_world()
print("LEVEL:", world.get_name() if world else "?")

readers, pickups, starts, triggers = [], [], [], []
for a in actors:
    cn = a.get_class().get_name()
    if cn.startswith("ZP_CardReaderPanel") or cn.startswith("BP_CardReaderPanel"):
        readers.append(a)
    elif "KeyPickup" in cn:
        pickups.append(a)
    elif "PlayerStart" in cn:
        starts.append(a)
    elif "ObjectiveTrigger" in cn:
        triggers.append(a)

print("=== CARD READERS (%d) ===" % len(readers))
for a in readers:
    loc = a.get_actor_location()
    try: oft = a.get_editor_property("ObjectiveFlagOnTry")
    except Exception as e: oft = "ERR:%s" % e
    try: ofu = a.get_editor_property("ObjectiveFlagOnUnlock")
    except Exception as e: ofu = "ERR:%s" % e
    try: req = a.get_editor_property("RequiredItemDA")
    except Exception as e: req = "ERR:%s" % e
    print("  %s @ (%.0f,%.0f,%.0f) ObjFlagOnTry=%s ObjFlagOnUnlock=%s RequiredItemDA=%s"
          % (a.get_actor_label(), loc.x, loc.y, loc.z, oft, ofu, req))

print("=== KEY PICKUPS (%d) ===" % len(pickups))
for a in pickups:
    loc = a.get_actor_location()
    try: item = a.get_editor_property("ItemDataAsset")
    except Exception as e: item = "ERR:%s" % e
    print("  %s @ (%.0f,%.0f,%.0f) Item=%s" % (a.get_actor_label(), loc.x, loc.y, loc.z, item))

print("=== PLAYER STARTS (%d) ===" % len(starts))
for a in starts:
    loc = a.get_actor_location()
    print("  %s @ (%.0f,%.0f,%.0f)" % (a.get_actor_label(), loc.x, loc.y, loc.z))

print("=== OBJECTIVE TRIGGERS (%d) ===" % len(triggers))
for a in triggers:
    print("  %s" % a.get_actor_label())
