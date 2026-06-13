"""Phase 1: match Lvl_Horror's occlusion. Lvl_Horror keeps Lumen GI ON (no GI
override), so walls occlude the skylight -> dark interiors. Building1_3rdFloor's
PPV_GlobalDarkness forced GI=NONE, removing occlusion -> skylight floods rooms.
Fix = turn OFF the GI-method override so the project default (Lumen) applies,
exactly like Lvl_Horror. Single, reversible change."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

w = ues.get_editor_world()
assert w and w.get_name() == "Building1_3rdFloor", "Wrong level: %s" % (w.get_name() if w else None)

gd = next((a for a in aus.get_all_level_actors()
           if isinstance(a, unreal.PostProcessVolume) and a.get_actor_label() == "PPV_GlobalDarkness"), None)
assert gd, "PPV_GlobalDarkness not found"

s = gd.get_editor_property("settings")
before_method = s.get_editor_property("dynamic_global_illumination_method")
before_override = s.get_editor_property("override_dynamic_global_illumination_method")
print("BEFORE: override_GI=%s method=%s" % (before_override, before_method))

# Turn off the override -> falls back to project-default GI (Lumen), matching Lvl_Horror.
s.set_editor_property("override_dynamic_global_illumination_method", False)
gd.set_editor_property("settings", s)

after = gd.get_editor_property("settings")
print("AFTER:  override_GI=%s (False => project default Lumen applies)"
      % after.get_editor_property("override_dynamic_global_illumination_method"))

saved = les.save_current_level()
print("SAVED:", saved)
