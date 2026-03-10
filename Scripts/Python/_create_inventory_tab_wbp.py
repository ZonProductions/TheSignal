import unreal

# Create WBP_InventoryTab widget blueprint
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.WidgetBlueprintFactory()

# Create the asset
path = '/Game/Blueprints/UI'
name = 'WBP_InventoryTab'

# Check if it already exists
existing = unreal.load_asset(f'{path}/{name}')
if existing:
    print(f'{name} already exists — skipping creation')
else:
    asset = asset_tools.create_asset(name, path, None, factory)
    if asset:
        print(f'Created {name} at {path}')
        print(f'Type: {type(asset)}')
    else:
        print('Failed to create asset')

# Now reparent to ZP_InventoryTabWidget via the generated class
# We'll use MCP reparent_blueprint for this
print('Asset created. Reparent to ZP_InventoryTabWidget via MCP next.')
