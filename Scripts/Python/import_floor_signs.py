"""
Re-import floor sign textures and rebuild decal material as opaque decal.
No translucency = no fade.
"""
import unreal

SRC_DIR = "C:/Users/Ommei/workspace/TheSignal/RawContent/Textures/FloorSigns"
DST_TEXTURE_PATH = "/Game/TheSignal/Textures/FloorSigns"
DST_MI_PATH = "/Game/TheSignal/Materials"

# --- Re-import textures ---
import_tasks = []
for i in range(1, 7):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", f"{SRC_DIR}/T_FloorSign_{i}.png")
    task.set_editor_property("destination_path", DST_TEXTURE_PATH)
    task.set_editor_property("destination_name", f"T_FloorSign_{i}")
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    import_tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(import_tasks)
print("Textures re-imported")

# --- Rebuild parent decal material ---
parent_path = f"{DST_MI_PATH}/M_FloorSignDecal"
parent_mat = unreal.load_asset(parent_path)

if parent_mat:
    parent_mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    parent_mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(parent_mat)

    tex_param = unreal.MaterialEditingLibrary.create_material_expression(
        parent_mat, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
    tex_param.set_editor_property("parameter_name", "BaseColor")

    unreal.MaterialEditingLibrary.connect_material_property(
        tex_param, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        parent_mat, unreal.MaterialExpressionConstant, -400, 200)
    roughness.set_editor_property("r", 0.8)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    unreal.MaterialEditingLibrary.recompile_material(parent_mat)
    unreal.EditorAssetLibrary.save_asset(parent_path)
    print("M_FloorSignDecal updated to opaque decal")

# --- Update MI textures ---
for i in range(1, 7):
    mi_path = f"{DST_MI_PATH}/MI_FloorSign_{i}"
    mi = unreal.load_asset(mi_path)
    if mi:
        tex = unreal.load_asset(f"{DST_TEXTURE_PATH}/T_FloorSign_{i}")
        if tex:
            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
                mi, "BaseColor", tex)
        unreal.EditorAssetLibrary.save_asset(mi_path)
        print(f"Updated: MI_FloorSign_{i}")

print("\nDone! No more fading.")
