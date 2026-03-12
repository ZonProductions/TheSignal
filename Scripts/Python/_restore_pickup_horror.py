import unreal

# Duplicate RPG variant to recreate Horror variant
source_path = "/Game/InventorySystemPro/ExampleContent/RPG/UI/Widgets/WBP_FirstTimePickupNotification_RPG"
target_name = "WBP_FirstTimePickupNotification_Horror"
target_folder = "/Game/InventorySystemPro/ExampleContent/Horror/UI/Widgets"
target_path = target_folder + "/" + target_name

# Check if target already exists
if unreal.EditorAssetLibrary.does_asset_exist(target_path):
    print(f"Target already exists: {target_path}")
else:
    # Load source
    src = unreal.load_asset(source_path)
    if not src:
        print(f"ERROR: Could not load source: {source_path}")
    else:
        # Duplicate using AssetTools (reliable for BPs)
        result = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            target_name, target_folder, src)
        if result:
            unreal.EditorAssetLibrary.save_asset(target_path)
            print(f"SUCCESS: Duplicated RPG -> Horror at {target_path}")
        else:
            print("ERROR: Duplication failed")
