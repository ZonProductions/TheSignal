import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.ZP_CrawlerBehaviorComponent):
        c.set_editor_property("hunt_speed", 220.0)
        c.set_editor_property("detection_range", 1000.0)
        a.modify()
        n += 1
try:
    unreal.EditorLevelLibrary.save_current_level()
except Exception as e:
    unreal.log_warning("save skip: %s" % str(e))
unreal.log_warning("CRAWLER_TUNE updated=%d (huntSpeed=320 detectRange=1000)" % n)
