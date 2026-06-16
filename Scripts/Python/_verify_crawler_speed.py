import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.ZP_CrawlerBehaviorComponent):
        unreal.log_warning("CRAWLER %s huntSpeed=%s detect=%s" % (a.get_actor_label(), c.get_editor_property("hunt_speed"), c.get_editor_property("detection_range")))
        break
