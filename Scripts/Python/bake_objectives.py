# Bake SourceData/Objectives.json -> /Game/Data/DT_Objectives. Run after editing the JSON.
# You author only "Id" per row; this derives the DataTable row Name from it.
import unreal, json
tools = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
PATH = "/Game/Data/DT_Objectives"
JSON = "C:/Users/Ommei/workspace/TheSignal/SourceData/Objectives.json"

if EAL.does_asset_exist(PATH):
    dt = unreal.load_asset(PATH)
else:
    f = unreal.DataTableFactory()
    f.set_editor_property("struct", unreal.ZP_ObjectiveDef.static_struct())
    dt = tools.create_asset("DT_Objectives", "/Game/Data", unreal.DataTable, f)

data = json.loads(open(JSON).read())
for row in data:
    row["Name"] = row.get("Id")          # DataTable row key derived from authored Id
json_str = json.dumps(data)

problems = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json_str)
print("fill problems:", problems if problems else "(none)")
print("rows:", unreal.DataTableFunctionLibrary.get_data_table_row_names(dt))
EAL.save_asset(PATH)
print("saved", PATH)
