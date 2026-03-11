"""Check what mesh BP_ItemContainer_Horror has on its inherited StaticMeshComponent."""
import unreal

for bp_path in ['/Game/InventorySystemPro/Blueprints/Actors/ItemContainer/BP_ItemContainer',
                '/Game/InventorySystemPro/ExampleContent/Horror/Blueprints/BP_ItemContainer_Horror']:
    bp = unreal.load_asset(bp_path)
    if not bp:
        print(f"NOT FOUND: {bp_path}")
        continue
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    sms = cdo.get_components_by_class(unreal.StaticMeshComponent)
    print(f"\n{bp.get_name()} — {len(sms)} StaticMeshComponents on CDO:")
    for sm in sms:
        print(f"  {sm.get_name()} mesh={sm.static_mesh}")

# Also check BP_LootLocker placed instances — spawn a temp one
bp_ll = unreal.load_asset('/Game/Blueprints/Gameplay/BP_LootLocker')
gen_ll = bp_ll.generated_class()
cdo_ll = unreal.get_default_object(gen_ll)
sms_ll = cdo_ll.get_components_by_class(unreal.StaticMeshComponent)
print(f"\nBP_LootLocker — {len(sms_ll)} StaticMeshComponents on CDO:")
for sm in sms_ll:
    print(f"  {sm.get_name()} mesh={sm.static_mesh}")
