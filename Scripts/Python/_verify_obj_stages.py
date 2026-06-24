import unreal
dt = unreal.load_asset("/Game/Data/DT_Objectives")
DFL = unreal.DataTableFunctionLibrary
print("ROWS:", DFL.get_data_table_row_names(dt))
try:
    j = DFL.get_data_table_as_json(dt)
    print("=== DT AS JSON ===")
    print(j)
except Exception as e:
    print("get_data_table_as_json unavailable:", e)
    print("=== SubObjectives column ===")
    print(DFL.get_data_table_column_as_string(dt, "SubObjectives"))
