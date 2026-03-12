import unreal
# Find Moonville inventory component class and its delegates/events
classes = unreal.EditorAssetLibrary.list_assets('/Game/InventorySystemPro/', recursive=True)
print(f"Total ISP assets: {len(classes)}")
# Look for inventory character component BP
for c in classes:
    if 'InventoryCharacter' in c or 'BP_Inventory' in c:
        print(f"  Found: {c}")
