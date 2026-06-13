import unreal
EAL = unreal.EditorAssetLibrary
print("referenced M_Dust_Particle (/Game/ModularSciFi/...):",
      EAL.does_asset_exist("/Game/ModularSciFi/Materials/VFX/Dust/M_Dust_Particle"))
# search for any dust particle material that DOES exist
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for cls in ["Material","MaterialInstanceConstant"]:
    assets = ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine",cls), True)
    for a in assets:
        n=str(a.asset_name).lower()
        if "dust" in n or ("mote" in n):
            print("  exists:", a.package_name, "(", cls, ")")
