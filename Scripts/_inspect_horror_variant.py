import unreal

print("=== Asset existence ===")
paths = [
    "/Game/Variant_Horror/Blueprints/Light/BP_HorrorLight",
    "/Game/Variant_Horror/Lvl_Horror",
    "/Game/Variant_Horror/Blueprints/Light/Assets/Materials/MI_FlickeringLight",
    "/Game/Variant_Horror/Blueprints/Light/Assets/Materials/MF_LightFlicker",
    "/Game/Variant_Horror/Blueprints/Light/Assets/Materials/LF_Flicker",
    "/Game/Variant_Horror/Blueprints/Light/Assets/NS_DustMote",
]
for p in paths:
    print(p, "->", unreal.EditorAssetLibrary.does_asset_exist(p))

try:
    w = unreal.EditorLevelLibrary.get_editor_world()
    print("CURRENT WORLD:", w.get_name())
except Exception as e:
    print("world err:", e)

# Inspect BP_HorrorLight components
bp = unreal.load_asset("/Game/Variant_Horror/Blueprints/Light/BP_HorrorLight")
print("\n=== BP_HorrorLight ===")
print("loaded:", bp is not None, type(bp).__name__ if bp else "")
if bp:
    gen = bp.generated_class()
    cdo = unreal.get_default_object(gen)
    print("parent:", gen.get_super_class().get_name())
    comps = unreal.SubobjectDataSubsystem
    # list components via CDO actor components
    try:
        for c in cdo.get_components_by_class(unreal.ActorComponent):
            print("  comp:", c.get_name(), "::", type(c).__name__)
    except Exception as e:
        print("comp err:", e)
