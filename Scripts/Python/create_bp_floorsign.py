"""Create BP_FloorSign as a Blueprint child of AZP_FloorSign (C++ class).
Blueprint actors rerun construction scripts on property changes,
which makes FloorNumber update the material automatically in the editor.
"""
import unreal

BP_NAME = "BP_FloorSign"
BP_PATH = "/Game/TheSignal/Blueprints/LevelDesign"

# Delete if it already exists
full_path = f"{BP_PATH}/{BP_NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full_path):
    unreal.EditorAssetLibrary.delete_asset(full_path)
    print(f"Deleted existing {BP_NAME}")

# Load C++ parent class
parent_class = unreal.load_class(None, "/Script/TheSignal.ZP_FloorSign")
if not parent_class:
    print("ERROR: Could not load ZP_FloorSign class")
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bp = asset_tools.create_asset(BP_NAME, BP_PATH, unreal.Blueprint, factory)

    if bp:
        unreal.EditorAssetLibrary.save_asset(full_path)
        print(f"SUCCESS: Created {BP_NAME} at {full_path}")
    else:
        print(f"ERROR: Failed to create {BP_NAME}")
