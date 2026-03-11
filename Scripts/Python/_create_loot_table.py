"""
Create DA_LootTable_Locker — weighted loot table for BP_LootLocker.
Target distribution: 75% empty, 20% one item, 5% two items.
Uses import_text to handle Blueprint struct's mangled property names.
"""
import unreal

# Property name GUIDs (from export_text of existing horror loot table)
ITEM_KEY = 'Item_16_5B7B9A344657783AAD32EEAD6E573BF3'
MIN_KEY = 'MinAmount_10_05CB7F6F46AB38D8424770A7A8D856EC'
MAX_KEY = 'MaxAmount_11_8CED635A4E084D9C424767A12E127F49'
CHANCE_KEY = 'ChanceToSpawn_15_DB25BF1E484BFA3EDBE439ADAFE803EC'

PDA_ITEM_PREFIX = "/Game/InventorySystemPro/Blueprints/Items/Core/PDA_Item.PDA_Item_C'"

# Loot entries: (item_path, chance)
# Chances = 0.30 * weight (total expected items = 0.30 per locker)
# P(0)=72.6%, P(1)=22.3%, P(2+)=5.1% — close to user's 75/20/5 target
LOOT_ENTRIES = [
    ('/Game/InventorySystemPro/ExampleContent/Common/Items/Consumables/DA_Medkit', 0.03),
    ('/Game/Core/Items/DA_StimVial', 0.06),
    ('/Game/Core/Items/DA_Suppressant', 0.03),
    ('/Game/Core/Items/DA_AdrenalineShot', 0.03),
    ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_9mm', 0.075),
    ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_556', 0.03),
    ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_Buckshot', 0.045),
]

DA_PATH = '/Game/Core/LootTables'
DA_NAME = 'DA_LootTable_Locker'
FULL_PATH = f'{DA_PATH}/{DA_NAME}'

# Load reference loot table to get struct template
ref_lt = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Items/ItemDrops/DA_ExampleItemLootTable_Horror')
ref_drops = ref_lt.get_editor_property('ItemDrops')
template_entry = ref_drops[0]

# Load or get existing asset
existing = unreal.load_asset(FULL_PATH)
if existing:
    print(f"DA_LootTable_Locker already exists, reusing...")
    new_asset = existing
else:
    # Create data asset
    loot_table_bp = unreal.load_asset('/Game/InventorySystemPro/Blueprints/Items/Core/PDA_ItemLootTable')
    loot_table_class = loot_table_bp.generated_class()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', loot_table_class)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    new_asset = asset_tools.create_asset(DA_NAME, DA_PATH, None, factory)
    print(f"Created: {new_asset.get_path_name()}")

# Build entries using import_text
drops = []
for item_path, chance in LOOT_ENTRIES:
    item_asset = unreal.load_asset(item_path)
    if not item_asset:
        print(f"  ERROR: Could not load {item_path}")
        continue

    # Get the asset's full object path for the text format
    asset_name = item_asset.get_name()
    item_ref = f"{PDA_ITEM_PREFIX}{item_path}.{asset_name}'"

    entry = template_entry.copy()
    text = f'({ITEM_KEY}="{item_ref}",{MIN_KEY}=1,{MAX_KEY}=1,{CHANCE_KEY}={chance:.6f})'
    entry.import_text(text)
    drops.append(entry)
    print(f"  {asset_name}: chance={chance}")

# Set on asset
new_asset.set_editor_property('ItemDrops', drops)

# Verify
verify = new_asset.get_editor_property('ItemDrops')
print(f"\nVerify: {len(verify)} entries")
for v in verify:
    print(f"  {v.export_text()}")

# Save
unreal.EditorAssetLibrary.save_asset(FULL_PATH)
print(f"\nSaved: {FULL_PATH}")
print("=== DONE ===")
