"""
WINDOW GLASS FIX (TICKET-052 adjacent) — Building1_3rdFloor.

Panes shipped as OPAQUE MI_ColorShinyBlackPlastic; MI_Glass_AO test swap
mirrored the dark interior in PIE (reflective glass + SSR = black mirror).

Builds M_WindowGlass: UNLIT translucent, opacity 0.12, two-sided.
Unlit = cannot be darkened by lighting/reflections — the night scene passes
through at ~88% no matter what. Applies to pane slots (0,1) on ALL
F3_SM_WindowWall actors, editor world + live PIE if running. Saves.
"""
import unreal

OPACITY = 0.12
PATH, NAME = '/Game/Core/Materials', 'M_WindowGlass'
PANE_SLOTS = (0, 1)
OLD_PANES = ('MI_ColorShinyBlackPlastic', 'MI_Glass_AO')

MEL = unreal.MaterialEditingLibrary
full = f'{PATH}/{NAME}'
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.load_asset(full)
    MEL.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)
op = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 0)
op.set_editor_property('r', OPACITY)
MEL.connect_material_property(op, '', unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)
unreal.log(f'{NAME} built (unlit translucent, opacity {OPACITY})')

def apply(world, tag):
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
        # ANY floor prefix — labels lie (F4_/F5_-labeled actors live on floor 3)
        if 'SM_WindowWall' in a.get_actor_label():
            for c in a.get_components_by_class(unreal.StaticMeshComponent):
                c.modify()
                for i in PANE_SLOTS:
                    cur = c.get_material(i)
                    if cur and (cur.get_name() in OLD_PANES or cur.get_name() == NAME):
                        c.set_material(i, mat)
                        n += 1
    unreal.log(f'{tag}: {n} pane slots -> {NAME}')

ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
apply(ues.get_editor_world(), 'editor')
gw = ues.get_game_world()
if gw:
    apply(gw, 'PIE (LIVE — visible in your current session)')
else:
    unreal.log('PIE not running — change is in the next Play')

ok = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
unreal.log(f'level saved: {ok}')
