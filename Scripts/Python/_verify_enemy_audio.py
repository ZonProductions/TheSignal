import unreal
c = unreal.get_default_object(unreal.ZP_EnemyAudioComponent)
f = lambda s: (s.get_name() if s else "NONE")
atks = c.get_editor_property("attack_sounds")
unreal.log_warning("ENEMY_AUDIO lurk=%s alert=%s attacks=[%s] attn=%s vol=%s" % (
    f(c.get_editor_property("lurking_loop")),
    f(c.get_editor_property("alert_sound")),
    ", ".join(f(a) for a in atks),
    f(c.get_editor_property("attenuation")),
    c.get_editor_property("volume_multiplier")))
