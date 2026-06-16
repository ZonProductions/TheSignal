import unreal
p = "/Game/Core/Data/DA_GraceMovement_Default"
d = unreal.load_asset(p)
d.set_editor_property("walk_speed", 260.0)          # 200 x1.3
d.set_editor_property("sprint_speed", 390.0)        # 300 x1.3
d.set_editor_property("crouch_walk_speed", 200.0)   # 100 x2
d.set_editor_property("stamina_regen_delay", 2.0)   # global 2s cooldown
unreal.EditorAssetLibrary.save_asset(p)
unreal.log_warning("DA_SET walk=%s sprint=%s crouch=%s delay=%s" % (
    d.get_editor_property("walk_speed"),
    d.get_editor_property("sprint_speed"),
    d.get_editor_property("crouch_walk_speed"),
    d.get_editor_property("stamina_regen_delay")))
