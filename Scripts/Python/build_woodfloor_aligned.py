"""
Build M_WoodFloorAligned — world-aligned wood floor material (session 63).

The 1x1m FloorTile grid (gameplay-carvable) shows texture seams at every
tile because each cube maps its own UVs. World-aligned projection makes the
wood grain continuous across ALL tiles and filler pieces — the floor reads
as one giant surface (like the ceiling) while the grid stays.

Tunables below. Re-run to rebuild.
"""
import unreal

PLANK_WORLD_SIZE = 400.0   # cm per texture repeat
ROUGHNESS = 0.8            # matte wood (dev: "normal wooden floors")
OUT_PATH = '/Game/Core/Materials'
OUT_NAME = 'M_WoodFloorAligned'
TEX_COLOR = '/Game/office_BigCompanyArchViz/Textures/T_Woodfloor2_BaseColor'
TEX_NORMAL = '/Game/office_BigCompanyArchViz/Textures/T_Woodfloor2_Normal'

MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()

full = f'{OUT_PATH}/{OUT_NAME}'
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.load_asset(full)
    # wipe and rebuild
    MEL.delete_all_material_expressions(mat)
else:
    mat = AT.create_asset(OUT_NAME, OUT_PATH, unreal.Material, unreal.MaterialFactoryNew())
assert mat, 'material create failed'

# --- BaseColor: WorldAlignedTexture(Color) ---
tex_c = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -900, -300)
tex_c.set_editor_property('parameter_name', 'WoodColor')
tex_c.set_editor_property('texture', unreal.load_asset(TEX_COLOR))

fn_c = MEL.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, -500, -300)
fn_c.set_editor_property('material_function',
    unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture'))

size_c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, -100)
size_c.set_editor_property('constant', unreal.LinearColor(PLANK_WORLD_SIZE, PLANK_WORLD_SIZE, PLANK_WORLD_SIZE, 1.0))

def connect_expr(src, src_pin, dst, dst_pin_variants):
    for v in dst_pin_variants:
        if MEL.connect_material_expressions(src, src_pin, dst, v):
            unreal.log(f'  OK: -> {v}')
            return True
    unreal.log_error(f'  FAILED: none of {dst_pin_variants} connected')
    return False

def connect_prop(src, src_pin_variants, prop):
    for v in src_pin_variants:
        if MEL.connect_material_property(src, v, prop):
            unreal.log(f'  OK: {v} -> {prop}')
            return True
    unreal.log_error(f'  FAILED: none of {src_pin_variants} -> {prop}')
    return False

connect_expr(tex_c, '', fn_c, ['TextureObject (T2d)', 'TextureObject', 'Texture Object (T2d)'])
connect_expr(size_c, '', fn_c, ['TextureSize (V3)', 'TextureSize', 'Texture Size (V3)'])
connect_prop(fn_c, ['XYZ Texture', 'XYZTexture'], unreal.MaterialProperty.MP_BASE_COLOR)

# --- Normal: WorldAlignedNormal(Normal) ---
tex_n = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -900, 200)
tex_n.set_editor_property('parameter_name', 'WoodNormal')
tex_n.set_editor_property('texture', unreal.load_asset(TEX_NORMAL))
tex_n.set_editor_property('sampler_type', unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)

fn_n = MEL.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, -500, 200)
fn_n.set_editor_property('material_function',
    unreal.load_asset('/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal'))

size_n = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -900, 400)
size_n.set_editor_property('constant', unreal.LinearColor(PLANK_WORLD_SIZE, PLANK_WORLD_SIZE, PLANK_WORLD_SIZE, 1.0))

connect_expr(tex_n, '', fn_n, ['TextureObject (T2d)', 'TextureObject', 'Texture Object (T2d)'])
connect_expr(size_n, '', fn_n, ['TextureSize (V3)', 'TextureSize', 'Texture Size (V3)'])
if not connect_prop(fn_n, ['XYZ Normal', 'XYZNormal', ''], unreal.MaterialProperty.MP_NORMAL):
    unreal.log_warning('Normal map left unconnected — flat shading (acceptable for matte wood)')

# --- Roughness: matte constant ---
rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 550)
rough.set_editor_property('r', ROUGHNESS)
MEL.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)

MEL.recompile_material(mat)
ok = unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)
unreal.log(f'{OUT_NAME} built and saved={ok}')
