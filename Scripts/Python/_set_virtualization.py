# Make the looping SignalSense stems "play when silent" so they keep advancing playback
# position at volume 0 and resume seamlessly (no cull/restart cut when re-entering a void).
import unreal

for name in ["MS_Signal_InterferenceBed", "SFX_Phone_Ring", "SFX_Signal_Alarm"]:
    p = "/Game/Audio/Signal/" + name
    w = unreal.load_asset(p)
    if not w:
        unreal.log_warning("VIRT_MISS " + name)
        continue
    try:
        w.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
        unreal.EditorAssetLibrary.save_asset(p)
        unreal.log_warning("VIRT_SET %s -> %s" % (name, str(w.get_editor_property("virtualization_mode"))))
    except Exception as e:
        unreal.log_warning("VIRT_ERR %s: %s" % (name, str(e)))
