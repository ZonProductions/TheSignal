# Imports the 5 SignalSense SFX from /Sfx into /Game/Audio/Signal, sets loop stems looping.
import unreal

src = "C:/Users/Ommei/workspace/TheSignal/Sfx"
dest = "/Game/Audio/Signal"
loop_stems = ["MS_Signal_InterferenceBed", "SFX_Phone_Notify", "SFX_Phone_Ring", "SFX_Signal_Alarm"]
files = loop_stems + ["SFX_Phone_Silence"]

tasks = []
for f in files:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src + "/" + f + ".wav")
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", True)
    t.set_editor_property("replace_existing", True)
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

# mark the 10.38s stems as looping where the property exists
for name in loop_stems:
    p = dest + "/" + name
    a = unreal.load_asset(p)
    if a:
        try:
            a.set_editor_property("looping", True)
            unreal.EditorAssetLibrary.save_asset(p)
        except Exception as e:
            unreal.log_warning("loop flag skip " + name + ": " + str(e))

# report
for f in files:
    p = dest + "/" + f
    unreal.log_warning("IMPORTED " + p + " exists=" + str(unreal.EditorAssetLibrary.does_asset_exist(p)))
