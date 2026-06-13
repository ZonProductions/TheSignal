"""Root cause: editor viewport Realtime was OFF, so UDS never ticked and its
sky/skylight stayed frozen at a bright build state (that's why every Python
refresh call no-op'd). Enable viewport Realtime so UDS's own editor tick runs
and recomputes night for Time of Day 200 (2 AM). UDS converges over the next
frames after this returns."""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

uds = next((a for a in aus.get_all_level_actors()
            if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"), None)
assert uds, "UDS not found"
uds.set_editor_property("Time of Day", 200.0)   # 2:00 AM

les.editor_set_viewport_realtime(True)
les.editor_invalidate_viewports()
print("Viewport Realtime = True; ToD = 200. UDS will tick to night now.")
print("(not saved yet — watch the viewport sweep to night, then I'll lock it in)")
