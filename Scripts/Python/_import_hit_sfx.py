import unreal
src = "C:/Users/Ommei/workspace/TheSignal/Sfx/CrawlerMono/SFX_Crawler_Hit.wav"
dest = "/Game/Audio/Crawler"
task = unreal.AssetImportTask()
task.filename = src
task.destination_path = dest
task.destination_name = "SFX_Crawler_Hit"
task.automated = True
task.replace_existing = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
a = unreal.load_asset("/Game/Audio/Crawler/SFX_Crawler_Hit")
if a:
    unreal.log_warning("HIT SFX imported: channels=%s" % a.get_editor_property("num_channels"))
else:
    unreal.log_warning("HIT SFX import FAILED")
