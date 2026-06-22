import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine","Blueprint")):
    n=str(ad.asset_name)
    if n.startswith("PC_") or "Grace" in n and "Controller" in n.lower() or n=="PC_Grace":
        print(n, "->", ad.package_name)
# also dump GM default PC class
gm = unreal.load_asset("/Game/Core/Framework/GM_TheSignal")
if gm:
    cdo = unreal.get_default_object(gm.generated_class())
    print("GM PlayerControllerClass:", cdo.get_editor_property("player_controller_class"))
