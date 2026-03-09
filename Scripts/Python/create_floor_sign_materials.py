"""
Create 6 floor sign materials as regular surface materials (NOT decals).
Parent: M_General from BigCompany pack. Apply to a thin plane placed on wall.
"""
import unreal

DST_TEXTURE_PATH = "/Game/TheSignal/Textures/FloorSigns"
DST_MI_PATH = "/Game/TheSignal/Materials"
PARENT = "/Game/office_BigCompanyArchViz/Materials/M_General"

parent_mat = unreal.load_asset(PARENT)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mi_factory = unreal.MaterialInstanceConstantFactoryNew()

# Delete old decal versions
for name in ["M_FloorSignDecal"]:
    p = f"{DST_MI_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(p):
        unreal.EditorAssetLibrary.delete_asset(p)
        print(f"Deleted decal material: {name}")

for i in range(1, 7):
    mi_name = f"MI_FloorSign_{i}"
    mi_path = f"{DST_MI_PATH}/{mi_name}"

    # Delete and recreate
    if unreal.EditorAssetLibrary.does_asset_exist(mi_path):
        unreal.EditorAssetLibrary.delete_asset(mi_path)

    mi = asset_tools.create_asset(mi_name, DST_MI_PATH,
                                   unreal.MaterialInstanceConstant, mi_factory)
    if mi:
        mi.set_editor_property("parent", parent_mat)

        tex = unreal.load_asset(f"{DST_TEXTURE_PATH}/T_FloorSign_{i}")
        if tex:
            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
                mi, "BaseColor", tex)

        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.6)
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "AO", 1000.0)

        unreal.EditorAssetLibrary.save_asset(mi_path)
        print(f"Created: {mi_name} (surface material)")

print("\nDone! Apply to a flat SM_Cube (scale 1,1,0.01) placed against the wall.")
