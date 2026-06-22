import unreal
dt = unreal.load_asset("/Game/Data/DT_Objectives")
DFL = unreal.DataTableFunctionLibrary
for col in ["Id","Title","bSideObjective","bStartOnLoad","Requires","SubObjectives"]:
    try:
        print(col, "=", DFL.get_data_table_column_as_string(dt, col))
    except Exception as e:
        print(col, "ERR", e)
