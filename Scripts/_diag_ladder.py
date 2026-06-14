import unreal

# 1) any ladder-named assets in content
unreal.log("=== ladder ASSETS in /Game ===")
for p in unreal.EditorAssetLibrary.list_assets("/Game", True, False):
    if "adder" in p:
        unreal.log("  " + p)

# 2) placed ladder actors in the current level + their class
unreal.log("=== placed ladder actors in level ===")
actors = unreal.EditorLevelLibrary.get_all_level_actors()
found = 0
for a in actors:
    cn = a.get_class().get_name()
    if "Ladder" in cn or "Ladder" in a.get_actor_label():
        loc = a.get_actor_location()
        unreal.log("  %s | class=%s | @(%.0f,%.0f,%.0f)" % (a.get_actor_label(), cn, loc.x, loc.y, loc.z))
        found += 1
unreal.log("placed ladder actors: %d" % found)

# 3) does the C++ class resolve, and is there a BP subclass?
cls = unreal.load_class(None, "/Script/TheSignal.ZP_Ladder")
unreal.log("ZP_Ladder C++ class loaded: %s" % (cls is not None))
