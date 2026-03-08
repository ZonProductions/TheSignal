import unreal

# Check what dialogue-related classes the plugin provides
ar = unreal.AssetRegistryHelpers.get_asset_registry()

# Look for plugin assets
unreal.log('=== Plugin content ===')
assets = ar.get_assets_by_path('/DialoguePlugin', recursive=True)
for a in assets:
    unreal.log(f'  {a.asset_name} ({a.asset_class_path.asset_name})')

# Check the plugin's base widget class more carefully - look for how it gets dialogue
# The widget probably calls a function on NPCActor to get the dialogue
# Let's check if plugin has an ActorComponent
unreal.log('=== Looking for dialogue components ===')
comps = ar.get_assets_by_path('/DialoguePlugin', recursive=True)
for c in comps:
    name = str(c.asset_name)
    if 'component' in name.lower() or 'npc' in name.lower() or 'actor' in name.lower():
        unreal.log(f'  MATCH: {name} ({c.asset_class_path.asset_name})')

# Check if there's a UDialogueComponent or similar C++ class
unreal.log('=== Checking C++ classes with Dialogue in name ===')
for cls_name in ['DialogueComponent', 'DialogueActorComponent', 'DialogueNPCComponent']:
    try:
        cls = unreal.find_class(cls_name)
        unreal.log(f'  FOUND: {cls_name} = {cls}')
    except:
        unreal.log(f'  NOT FOUND: {cls_name}')
