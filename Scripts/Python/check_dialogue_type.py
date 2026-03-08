import unreal
asset = unreal.load_asset('/Game/Dialogue/Dialogue_Test')
if asset:
    print(f"Class: {asset.get_class().get_name()}")
    print(f"Parent: {asset.get_class().get_super_class().get_name()}")
else:
    print("Asset not found")
