import unreal

bp = unreal.load_asset("/Game/InventorySystemPro/Blueprints/Input/Common/BP_InventoryUIInputData")
cls = bp.generated_class() if hasattr(bp, "generated_class") else bp
cdo = unreal.get_default_object(cls)
print("CDO:", cdo.get_name())

def dump_handle(label, h):
    try:
        dt = h.get_editor_property("data_table")
        rn = h.get_editor_property("row_name")
        print("  %s -> table=%s row=%s" % (label, dt.get_name() if dt else None, rn))
        return dt, rn
    except Exception as e:
        print("  %s err: %s" % (label, e))
        return None, None

dt = None
for prop in ["default_click_action", "default_back_action"]:
    try:
        h = cdo.get_editor_property(prop)
        d, rn = dump_handle(prop, h)
        if d: dt = d
    except Exception as e:
        print("  no", prop, e)

# Dump all rows of the action data table with their key bindings
if dt:
    print("\nData table:", dt.get_package_name() if hasattr(dt,"get_package_name") else dt.get_name())
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)
    for rn in row_names:
        d = unreal.DataTableFunctionLibrary.get_data_table_row_as_string(dt, rn) if hasattr(unreal.DataTableFunctionLibrary,"get_data_table_row_as_string") else None
        print("  ROW", rn, "->", d)
