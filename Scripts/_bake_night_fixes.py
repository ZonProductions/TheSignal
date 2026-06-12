"""BAKE (run with PIE stopped — this script ends PIE first):
1. End PIE
2. Occlusion: duplicate pack settings -> /Game/Core/DA_UDS_Occlusion_Off
   (volume-test mode + fractions 0), assign to editor UDS
3. UDS Sky Light Color Multiplier (Night) = (3, 3, 3.5) — dev-approved
4. Save level + assets"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.editor_request_end_play()
unreal.log('PIE end requested')

SRC = '/Game/UltraDynamicSky/Blueprints/Occlusion/Occlusion_Settings/UDS_Standard_Occlusion_Settings'
DST = '/Game/Core/DA_UDS_Occlusion_Off'

eal = unreal.EditorAssetLibrary
if not eal.does_asset_exist(DST):
    ok = eal.duplicate_asset(SRC, DST)
    unreal.log(f'duplicate -> {ok}')
da = unreal.load_asset(DST)
mode_cls = type(da.get_editor_property('Occlusion Mode'))
da.set_editor_property('Occlusion Mode', mode_cls.TEST_IF_INSIDE_OCCLUSION_VOLUME)
da.set_editor_property('Global Occlusion Min Fraction', 0.0)
da.set_editor_property('Global Occlusion Max Fraction', 0.0)
unreal.log(f'DA_UDS_Occlusion_Off: mode={da.get_editor_property("Occlusion Mode")}, fractions 0')

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w = ues.get_editor_world()
u = [a for a in unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
     if 'Ultra_Dynamic_Sky' in a.get_class().get_name()][0]
u.modify()
u.set_editor_property('Occlusion Settings', da)
u.set_editor_property('Sky Light Color Multiplier (Night)',
                      unreal.LinearColor(3.0, 3.0, 3.5, 1.0))
u.call_method('\U0001F4D8Time of Day')
unreal.log('editor UDS: occlusion-off DA assigned, night multiplier (3,3,3.5)')

ok1 = eal.save_asset(DST, only_if_is_dirty=False)
ok2 = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'saved: asset={ok1} packages={ok2}')
