"""Swimming-pool on wall reflections = Screen-Space Reflections (SSR) smearing.
PPV_GlobalDarkness forces reflection_method=SCREEN_SPACE (set back when Lumen GI
was OFF). Lumen GI is on again now, so switch reflections to LUMEN (stable, no
swimming). NOT a DEAD-END repeat — the old swimming-pool work only toggled GI,
never the reflection method. Keeps GI on, so no interior flooding returns."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

w = ues.get_editor_world()
assert w and w.get_name() == "Building1_3rdFloor", "Wrong/none level: %s" % (w.get_name() if w else None)

gd = next((a for a in aus.get_all_level_actors()
           if isinstance(a, unreal.PostProcessVolume) and a.get_actor_label() == "PPV_GlobalDarkness"), None)
assert gd, "PPV_GlobalDarkness not found"

s = gd.get_editor_property("settings")
print("BEFORE reflection_method:", s.get_editor_property("reflection_method"))
s.set_editor_property("reflection_method", unreal.ReflectionMethod.LUMEN)
gd.set_editor_property("settings", s)
print("AFTER  reflection_method:", gd.get_editor_property("settings").get_editor_property("reflection_method"))

saved = les.save_current_level()
print("SAVED:", saved)
