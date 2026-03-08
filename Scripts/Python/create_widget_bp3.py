import unreal

package_path = "/Game/Blueprints/Widgets"
asset_name = "WBP_DesignNameOverlay"

# Try BlueprintFactory with UserWidget parent
factory = unreal.BlueprintFactory()
factory.set_editor_property("ParentClass", unreal.UserWidget)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
new_asset = asset_tools.create_asset(asset_name, package_path, None, factory)
print(f"Created: {new_asset}")
if new_asset:
    print(f"Asset class: {new_asset.get_class().get_name()}")
    # Now reparent to ZP_DesignNameWidget
    gen = new_asset.generated_class()
    print(f"Gen class: {gen}")
    unreal.EditorAssetLibrary.save_asset(f"{package_path}/{asset_name}")
    print("Saved")
else:
    # Try WidgetBlueprint class directly
    print("BlueprintFactory failed, trying to find WidgetBlueprint factories...")
    for attr in dir(unreal):
        if "widget" in attr.lower() and "factory" in attr.lower():
            print(f"  Found: {attr}")
