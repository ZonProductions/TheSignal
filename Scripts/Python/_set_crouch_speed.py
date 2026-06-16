import unreal
p = "/Game/Core/Data/DA_GraceMovement_Default"
d = unreal.load_asset(p)
d.set_editor_property("crouch_walk_speed", 150.0)
unreal.EditorAssetLibrary.save_asset(p)
unreal.log_warning("CROUCH_SET=%s" % d.get_editor_property("crouch_walk_speed"))
