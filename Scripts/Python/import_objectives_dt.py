import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()

task = unreal.AssetImportTask()
task.filename = "C:/Users/Ommei/workspace/TheSignal/SourceData/Objectives.json"
task.destination_path = "/Game/Data"
task.destination_name = "DT_Objectives"
task.replace_existing = True
task.automated = True
task.save = True

factory = unreal.CSVImportFactory()
s = factory.get_editor_property("automated_import_settings")
s.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)
s.set_editor_property("import_row_struct", unreal.ZP_ObjectiveDef.static_struct())
factory.set_editor_property("automated_import_settings", s)
task.factory = factory

tools.import_asset_tasks([task])

dt = unreal.load_asset("/Game/Data/DT_Objectives")
print("DT_Objectives:", dt)
if dt:
    names = unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)
    print("row names:", names)
    unreal.EditorAssetLibrary.save_asset("/Game/Data/DT_Objectives")
