"""Inspect UDS occlusion mode value/type + find the enum asset."""
import unreal

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_game_world() or ues.get_editor_world()
unreal.log(f'world={w.get_name() if w else None}')
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
os_ = u.get_editor_property('Occlusion Settings')
mode = os_.get_editor_property('Occlusion Mode')
unreal.log(f'value={mode!r} type={type(mode)} obj={os_.get_path_name()}')

reg = unreal.AssetRegistryHelpers.get_asset_registry()
for ad in reg.get_assets_by_class(unreal.TopLevelAssetPath('/Script/Engine', 'UserDefinedEnum'), True):
    name = str(ad.asset_name)
    if 'Occlusion' in name:
        unreal.log(f'enum asset: {ad.package_name}')
        e = unreal.load_asset(str(ad.package_name))
        try:
            n = e.get_editor_property('enum_size') if False else None
        except Exception:
            pass
        # dump display names
        for i in range(16):
            try:
                dn = e.get_display_name_text_by_index(i)
                unreal.log(f'  [{i}] {dn}')
            except Exception:
                break
