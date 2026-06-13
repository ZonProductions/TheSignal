import unreal
# Grenade icon texture validity
t = unreal.load_asset("/Game/icons/Icon_Grenade")
print("Icon_Grenade tex:", isinstance(t, unreal.Texture2D),
      t.blueprint_get_size_x() if t else None, "never_stream",
      t.get_editor_property("never_stream") if t else None)

# HUD CDO grenade texture ref
bp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
cdo = unreal.get_default_object(bp.generated_class())
gt = cdo.get_editor_property("GrenadeIconTexture")
print("CDO GrenadeIconTexture:", gt.get_name() if gt else None)

# Find the grenade weapon class name (what ApplyWeaponConfig sees)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
print("=== assets with 'Grenade' in name (weapon classes / DAs) ===")
for ad in ar.get_assets_by_path("/Game/Core", recursive=True, include_only_on_disk_assets=False):
    n = str(ad.asset_name)
    if "Grenade" in n or "Explosive" in n:
        print("  ", ad.asset_class_path.asset_name, n, "->", ad.package_name)
