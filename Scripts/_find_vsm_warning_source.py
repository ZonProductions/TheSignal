import unreal

aus = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
uds = next((a for a in aus.get_all_level_actors()
            if a.get_class().get_name() == "Ultra_Dynamic_Sky_C"), None)

# UDS properties that look like warning / message / editor-display toggles
if uds:
    props = unreal.EditorAssetLibrary  # noop ref
    hits = []
    for p in dir(uds):
        lp = p.lower()
        if any(k in lp for k in ("warn", "message", "hide", "suppress", "editor_only",
                                 "show", "notif", "vsm", "shadow")):
            hits.append(p)
    print("UDS dir() candidates:", hits)

# Try to read likely-named UDS warning toggle properties (display names)
for name in ["Show Editor Warnings", "Display Editor Warnings", "Hide Editor Warnings",
             "Suppress Warnings", "Show Warnings", "Disable Editor Warnings",
             "Editor Warnings", "Show On Screen Messages", "Hide On Screen Debug"]:
    try:
        print("  PROP", repr(name), "=", uds.get_editor_property(name))
    except Exception:
        pass

# Relevant VSM console variables (engine on-screen warnings)
for cv in ["r.Shadow.Virtual.ShowStats", "r.Shadow.Virtual.Cache.ShowStats",
           "r.Shadow.Virtual.NonNaniteVSM"]:
    try:
        v = unreal.SystemLibrary.get_console_variable_float_value(cv)
        print("  CVAR", cv, "=", v)
    except Exception as e:
        print("  CVAR", cv, "read-fail", str(e)[:40])
