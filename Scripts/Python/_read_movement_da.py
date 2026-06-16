import unreal
d = unreal.load_asset("/Game/Core/Data/DA_GraceMovement_Default")
if not d:
    unreal.log_warning("DA_MISSING")
else:
    unreal.log_warning("DA walk=%s sprint=%s crouch=%s regenDelay=%s regenRate=%s drain=%s" % (
        d.get_editor_property("walk_speed"),
        d.get_editor_property("sprint_speed"),
        d.get_editor_property("crouch_walk_speed"),
        d.get_editor_property("stamina_regen_delay"),
        d.get_editor_property("stamina_regen_rate"),
        d.get_editor_property("stamina_drain_rate")))
