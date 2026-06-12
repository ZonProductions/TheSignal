"""Kill the UDS occlusion box.
Duplicates UDS_Standard_Occlusion_Settings -> /Game/Core/DA_UDS_Occlusion_Off
(mode=Off), assigns it to the UDS actor in editor world + live PIE, fires the
UDS update event. Saves editor level + new asset."""
import unreal

SRC = '/Game/UltraDynamicSky/Blueprints/Occlusion/Occlusion_Settings/UDS_Standard_Occlusion_Settings'
DST_PATH, DST_NAME = '/Game/Core', 'DA_UDS_Occlusion_Off'
DST = f'{DST_PATH}/{DST_NAME}'

eal = unreal.EditorAssetLibrary
if not eal.does_asset_exist(DST):
    assert eal.duplicate_asset(SRC, DST), 'duplicate failed'
da = unreal.load_asset(DST)
mode_cls = type(da.get_editor_property('Occlusion Mode'))
unreal.log(f'enum entries: {[str(v) for v in mode_cls]}')
off = mode_cls(0)
da.set_editor_property('Occlusion Mode', off)
unreal.log(f'{DST_NAME}: Occlusion Mode = {da.get_editor_property("Occlusion Mode")}')
eal.save_asset(DST, only_if_is_dirty=False)

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

def apply(world, tag):
    if not world:
        unreal.log_warning(f'{tag}: no world')
        return
    uds = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
           if 'Ultra_Dynamic_Sky' in a.get_class().get_name()]
    if not uds:
        unreal.log_warning(f'{tag}: no UDS')
        return
    u = uds[0]
    u.modify()
    u.set_editor_property('Occlusion Settings', da)
    u.call_method('\U0001F4D8Time of Day')
    cur = u.get_editor_property('Occlusion Settings')
    unreal.log(f'{tag}: UDS Occlusion Settings -> {cur.get_name()} '
               f'(mode={cur.get_editor_property("Occlusion Mode")})')

apply(ues.get_editor_world(), 'EDITOR')
apply(ues.get_game_world(), 'PIE (LIVE — visible in your session now)')

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'saved: {ok}')
