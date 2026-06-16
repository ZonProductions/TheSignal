import unreal
c = unreal.get_default_object(unreal.ZP_CrawlerBehaviorComponent)
f = lambda s: (s.get_name() if s else "NONE")
unreal.log_warning("CRAWLER_AUDIO lurk=%s alert=%s atk1=%s atk2=%s falloff=%s" % (
    f(c.get_editor_property("lurking_loop")),
    f(c.get_editor_property("alert_sound")),
    f(c.get_editor_property("attack_sound1")),
    f(c.get_editor_property("attack_sound2")),
    c.get_editor_property("sound_falloff_distance")))
