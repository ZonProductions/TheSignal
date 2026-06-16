import unreal
cdo = unreal.get_default_object(unreal.ZP_GraceCharacter)
c = cdo.get_editor_property("signal_sense_comp")
f = lambda s: (s.get_name() if s else "NONE")
if not c:
    unreal.log_warning("SS_CHECK: component MISSING on CDO")
else:
    unreal.log_warning("SS_CHECK comp=OK interf=%s ring=%s alarm=%s clear=%s | enemyR=%s meleeR=%s kneeP=%s kneeV=%s" % (
        f(c.get_editor_property("interference_loop")),
        f(c.get_editor_property("ring_loop")),
        f(c.get_editor_property("alarm_loop")),
        f(c.get_editor_property("clear_sting")),
        c.get_editor_property("enemy_radius"),
        c.get_editor_property("melee_radius"),
        c.get_editor_property("interference_knee_proximity"),
        c.get_editor_property("interference_knee_volume")))
