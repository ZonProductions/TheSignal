import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = aus.get_all_level_actors()

print("=== UDS / Sky actor counts ===")
from collections import Counter
cc = Counter(a.get_class().get_name() for a in actors
             if "Sky" in a.get_class().get_name() or "Ultra" in a.get_class().get_name())
print(dict(cc))

print("=== Every SkyLight component (owner actor name + label) ===")
for a in actors:
    for c in a.get_components_by_class(unreal.SkyLightComponent):
        st = c.get_editor_property("source_type")
        cube = None
        try:
            cm = c.get_editor_property("cubemap")
            cube = cm.get_path_name() if cm else None
        except Exception:
            pass
        print(" actor:%s (class %s) | comp:%s | intensity:%s realtime:%s source:%s cubemap:%s visible:%s"
              % (a.get_actor_label(), a.get_class().get_name(), c.get_name(),
                 c.get_editor_property("intensity"),
                 c.get_editor_property("real_time_capture"),
                 st, cube, c.get_editor_property("visible")))

print("=== Time of Day re-read ===")
uds = [a for a in actors if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"]
for u in uds:
    print(" UDS actor:", u.get_actor_label(), "| Time of Day =", u.get_editor_property("Time of Day"))
