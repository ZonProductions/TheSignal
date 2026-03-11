"""
Setup BP_LootLocker: configure ItemsToAdd and ItemsToAddAmount on CDO.
"""
import unreal

BP_PATH = '/Game/Blueprints/Gameplay/BP_LootLocker'

# Item paths to add as test loot
ITEMS = [
    '/Game/InventorySystemPro/ExampleContent/Common/Items/Consumables/DA_Medkit',
    '/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_9mm',
    '/Game/Core/Items/DA_StimVial',
]
AMOUNTS = [1, 2, 1]  # 1 medkit, 2 ammo boxes, 1 stim vial

# Load BP and get CDO
bp = unreal.load_asset(BP_PATH)
if not bp:
    print("ERROR: BP_LootLocker not found!")
else:
    gen_class = bp.generated_class()
    cdo = unreal.get_default_object(gen_class)
    print(f"CDO: {cdo}")

    # Load item data assets
    items_array = []
    for item_path in ITEMS:
        item = unreal.load_asset(item_path)
        if item:
            print(f"  Loaded: {item.get_name()} ({item.__class__.__name__})")
            items_array.append(item)
        else:
            print(f"  ERROR: Could not load {item_path}")

    # Try to set ItemsToAdd
    try:
        cdo.set_editor_property('ItemsToAdd', items_array)
        print(f"ItemsToAdd set: {len(items_array)} items")
    except Exception as e:
        print(f"set_editor_property('ItemsToAdd') failed: {e}")
        # Try lowercase
        try:
            cdo.set_editor_property('items_to_add', items_array)
            print(f"items_to_add set: {len(items_array)} items")
        except Exception as e2:
            print(f"items_to_add also failed: {e2}")

    # Try to set ItemsToAddAmount
    try:
        cdo.set_editor_property('ItemsToAddAmount', AMOUNTS)
        print(f"ItemsToAddAmount set: {AMOUNTS}")
    except Exception as e:
        print(f"set_editor_property('ItemsToAddAmount') failed: {e}")
        try:
            cdo.set_editor_property('items_to_add_amount', AMOUNTS)
            print(f"items_to_add_amount set: {AMOUNTS}")
        except Exception as e2:
            print(f"items_to_add_amount also failed: {e2}")

    # Verify
    try:
        result = cdo.get_editor_property('ItemsToAdd')
        print(f"Verify ItemsToAdd: {result}")
    except:
        try:
            result = cdo.get_editor_property('items_to_add')
            print(f"Verify items_to_add: {result}")
        except:
            print("Could not verify ItemsToAdd")

    # Save
    unreal.EditorAssetLibrary.save_asset(BP_PATH)
    print("Saved BP_LootLocker")

print("\n=== DONE ===")
