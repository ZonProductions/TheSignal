import unreal
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
print("Level:", w.get_name() if w else None)
vols = [a for a in ss.get_all_level_actors() if "ZP_MapVolume" in a.get_class().get_name()]
print("MapVolumes:", len(vols))
for v in vols:
    has_tex = v.get_editor_property("map_texture") is not None
    print("  ", v.get_actor_label(), "| AreaID =", str(v.get_editor_property("area_id")),
          "| MapTexture:", "YES" if has_tex else "none")
