import unreal

png_path = "C:/Users/Ommei/workspace/TheSignal/Scripts/DevTools/../FloorPlans/F5_map.png"
asset_name = "T_Map_F5"
dest_path = "/Game/Core/Maps"

if not unreal.EditorAssetLibrary.does_directory_exist(dest_path):
    unreal.EditorAssetLibrary.make_directory(dest_path)

task = unreal.AssetImportTask()
task.set_editor_property("filename", png_path)
task.set_editor_property("destination_path", dest_path)
task.set_editor_property("destination_name", asset_name)
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

imported = unreal.load_asset(dest_path + "/" + asset_name)
if imported:
    print("SUCCESS: Imported " + asset_name + " to " + dest_path)
else:
    print("FAILED: Could not import " + asset_name)
