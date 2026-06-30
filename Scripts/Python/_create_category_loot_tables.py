"""
Create DA_LootTable_Ammo and DA_LootTable_Wellness — category-specific loot tables
for BP_LootLocker (General stays DA_LootTable_Locker).

Same PDA_ItemLootTable / ItemDrops format as _create_loot_table.py.
Per-item chances are starting points (scaled from the approved general weights so a
deliberately-placed themed cache averages ~1 item) — TUNE per design.
Ammo-gating is unchanged: FilterLockerAmmo() strips ammo the player can't use at open.
"""
import unreal

# Mangled struct property keys (from the existing horror loot table template)
ITEM_KEY = 'Item_16_5B7B9A344657783AAD32EEAD6E573BF3'
MIN_KEY = 'MinAmount_10_05CB7F6F46AB38D8424770A7A8D856EC'
MAX_KEY = 'MaxAmount_11_8CED635A4E084D9C424767A12E127F49'
CHANCE_KEY = 'ChanceToSpawn_15_DB25BF1E484BFA3EDBE439ADAFE803EC'
PDA_ITEM_PREFIX = "/Game/InventorySystemPro/Blueprints/Items/Core/PDA_Item.PDA_Item_C'"

DA_PATH = '/Game/Core/LootTables'

# table_name -> [(item_path, chance), ...]
TABLES = {
    'DA_LootTable_Ammo': [
        ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_9mm', 0.50),
        ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_556', 0.20),
        ('/Game/InventorySystemPro/ExampleContent/Common/Items/Ammo/DA_Ammo_Buckshot', 0.30),
    ],
    'DA_LootTable_Wellness': [
        ('/Game/InventorySystemPro/ExampleContent/Common/Items/Consumables/DA_Medkit', 0.15),
        ('/Game/Core/Items/DA_StimVial', 0.30),
        ('/Game/Core/Items/DA_Suppressant', 0.15),
        ('/Game/Core/Items/DA_AdrenalineShot', 0.15),
    ],
}

# Struct template from a known-good loot table
ref_lt = unreal.load_asset('/Game/InventorySystemPro/ExampleContent/Common/Items/ItemDrops/DA_ExampleItemLootTable_Horror')
template_entry = ref_lt.get_editor_property('ItemDrops')[0]

loot_table_bp = unreal.load_asset('/Game/InventorySystemPro/Blueprints/Items/Core/PDA_ItemLootTable')
loot_table_class = loot_table_bp.generated_class()
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

for da_name, entries in TABLES.items():
    full = f'{DA_PATH}/{da_name}'
    existing = unreal.load_asset(full)
    if existing:
        print(f'{da_name} already exists — mutating in place (no overwrite/recreate)')
        new_asset = existing
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', loot_table_class)
        new_asset = asset_tools.create_asset(da_name, DA_PATH, None, factory)
        print(f'Created: {new_asset.get_path_name()}')

    drops = []
    for item_path, chance in entries:
        item_asset = unreal.load_asset(item_path)
        if not item_asset:
            print(f'  ERROR: could not load {item_path}')
            continue
        item_ref = f"{PDA_ITEM_PREFIX}{item_path}.{item_asset.get_name()}'"
        entry = template_entry.copy()
        entry.import_text(f'({ITEM_KEY}="{item_ref}",{MIN_KEY}=1,{MAX_KEY}=1,{CHANCE_KEY}={chance:.6f})')
        drops.append(entry)
        print(f'  {item_asset.get_name()}: chance={chance}')

    new_asset.set_editor_property('ItemDrops', drops)
    verify = new_asset.get_editor_property('ItemDrops')
    print(f'  -> {da_name}: {len(verify)} entries')
    saved = unreal.EditorAssetLibrary.save_asset(full)
    print(f'  saved={saved}')

print('=== DONE ===')
