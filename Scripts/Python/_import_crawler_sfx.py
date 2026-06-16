import unreal

src = "C:/Users/Ommei/workspace/TheSignal/Sfx"
dest = "/Game/Audio/Crawler"
# filename (no ext)  ->  clean asset name
files = {
    "Signal Sfx_Crawler_Lurking": "SFX_Crawler_Lurking",
    "Signal Sfx_Crawler_Alert":   "SFX_Crawler_Alert",
    "Signal Sfx_Crawler_Attack":  "SFX_Crawler_Attack",
    "Signal Sfx_Crawler_Attack2": "SFX_Crawler_Attack2",
}

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

EAL = unreal.EditorAssetLibrary
for f, clean in files.items():
    san = dest + "/" + f.replace(" ", "_")          # UE sanitizes spaces -> _
    new = dest + "/" + clean
    if EAL.does_asset_exist(san) and not EAL.does_asset_exist(new):
        EAL.rename_asset(san, new)

# Lurking is a continuous bed -> loop + play-when-silent
lk = unreal.load_asset(dest + "/SFX_Crawler_Lurking")
if lk:
    try:
        lk.set_editor_property("looping", True)
        lk.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    except Exception as e:
        unreal.log_warning("lurk prop skip: %s" % str(e))
    EAL.save_asset(dest + "/SFX_Crawler_Lurking")

unreal.log_warning("CRAWLER_SFX_DONE: " + ", ".join(
    "%s=%s" % (c, EAL.does_asset_exist(dest + "/" + c)) for c in files.values()))
