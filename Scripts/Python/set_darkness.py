"""
THE brightness knob for Maps_BigCompany (and any map with PPV_GlobalDarkness).

Edit VALUE, re-run via MCP. Applies live to the loaded level — no rebuild,
no restart. SAVE THE LEVEL after settling on a value.

Scale (with the session-63 fixed exposure 0.8-1.2):
  0.0025 = original TreatmentStation horror crush (very dark)
  0.005  = dark, silhouettes readable
  0.01   = dim interior
  0.02   = visibly lit
  0.05+  = bright office
"""
import unreal

VALUE = 0.0025

es = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = es.get_editor_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.PostProcessVolume):
    if a.get_actor_label() == 'PPV_GlobalDarkness':
        s = a.get_editor_property('settings')
        s.set_editor_property('override_indirect_lighting_intensity', True)
        s.set_editor_property('indirect_lighting_intensity', VALUE)
        a.set_editor_property('settings', s)
        a.modify()
        unreal.log(f'[{w.get_name()}] indirect_lighting_intensity = {VALUE}')
        break
else:
    unreal.log_warning('PPV_GlobalDarkness not found in loaded level')
