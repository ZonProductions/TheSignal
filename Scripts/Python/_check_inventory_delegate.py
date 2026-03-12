"""Verify OnInventoryUpdate property exists on BP_InventoryCharacterComponent_C via reflection."""
import unreal

bp = unreal.load_asset('/Game/InventorySystemPro/Blueprints/Player/BP_InventoryCharacterComponent')
gen = bp.generated_class()
cdo = unreal.get_default_object(gen)

# Check all properties containing 'Inventory' or 'Update'
print(f'Class: {gen.get_name()}')
print(f'Super: {gen.get_super_class().get_name()}')

# Walk the class hierarchy
cls = gen
depth = 0
while cls and depth < 10:
    print(f'  [{depth}] {cls.get_name()}')
    cls = cls.get_super_class()
    depth += 1
