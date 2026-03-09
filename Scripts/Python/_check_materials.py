import unreal

for i in range(1, 7):
    mi_path = f"/Game/TheSignal/Materials/MI_FloorSign_{i}"
    mi = unreal.load_asset(mi_path)
    if mi:
        print(f"MI_FloorSign_{i}: EXISTS, class={mi.__class__.__name__}")
        # Check parent
        parent = mi.get_editor_property("parent")
        print(f"  Parent: {parent.get_path_name() if parent else 'NONE'}")
        # Check texture parameters
        tex_params = unreal.MaterialEditingLibrary.get_material_instance_texture_parameter_value(mi, "BaseColor")
        print(f"  BaseColor texture: {tex_params.get_path_name() if tex_params else 'NONE'}")
    else:
        print(f"MI_FloorSign_{i}: MISSING")

# Also check what texture parameters M_General has
parent_path = "/Game/office_BigCompanyArchViz/Materials/M_General"
parent_mat = unreal.load_asset(parent_path)
if parent_mat:
    print(f"\nM_General exists, checking parameter names...")
    # List all texture parameter names
    info = unreal.MaterialEditingLibrary.get_statistics(parent_mat)
    print(f"  Stats: {info}")
else:
    print(f"\nM_General: MISSING at {parent_path}")
