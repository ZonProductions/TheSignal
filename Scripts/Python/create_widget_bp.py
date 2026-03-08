import unreal

package_path = "/Game/Blueprints/Widgets"
asset_name = "WBP_DesignNameInput"

# Get the parent C++ class
parent_class = unreal.load_object(None, "/Script/TheSignal.ZP_DesignNameWidget")
print(f"Parent class: {parent_class}")

# Create WidgetBlueprint using the factory
factory = unreal.WidgetBlueprintFactory()
factory.set_editor_property("ParentClass", parent_class)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
new_asset = asset_tools.create_asset(asset_name, package_path, None, factory)
print(f"Created: {new_asset}")
print(f"Asset class: {new_asset.get_class().get_name() if new_asset else 'FAILED'}")

if new_asset:
    unreal.EditorAssetLibrary.save_asset(f"{package_path}/{asset_name}")
    print("Saved")
