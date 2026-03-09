import unreal

# Check M_ProbBase parent material for UV tiling/offset parameters
mat = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/M_ProbBase')
if not mat:
    print('M_ProbBase not found')
else:
    print(f'M_ProbBase: {mat.get_class().get_name()}')

    # Get all expressions to understand what parameters are available
    mel = unreal.MaterialEditingLibrary
    exprs = mel.get_used_textures(mat)
    print(f'Used textures: {[t.get_name() for t in exprs]}')

# Check MI_WCPackB for all parameter types
mi = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_WCPackB')
if mi:
    print(f'\nMI_WCPackB parameters:')

    scalars = mi.get_editor_property('scalar_parameter_values')
    for sp in scalars:
        pinfo = sp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = sp.get_editor_property('parameter_value')
        print(f'  Scalar: {pname} = {pval}')

    vectors = mi.get_editor_property('vector_parameter_values')
    for vp in vectors:
        pinfo = vp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = vp.get_editor_property('parameter_value')
        print(f'  Vector: {pname} = ({pval.r:.3f}, {pval.g:.3f}, {pval.b:.3f}, {pval.a:.3f})')

    textures = mi.get_editor_property('texture_parameter_values')
    for tp in textures:
        pinfo = tp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = tp.get_editor_property('parameter_value')
        print(f'  Texture: {pname} = {pval.get_name() if pval else "None"}')

# Also check what scalar params M_ProbBase exposes (UV tiling etc)
# by looking at another MI that might override tiling
print('\n--- Checking M_ProbBase scalar param defaults ---')
# Get info from the material's parameter list
if mat:
    info = unreal.MaterialEditingLibrary.get_scalar_parameter_names(mat)
    print(f'Scalar param names: {list(info)}')
    info2 = unreal.MaterialEditingLibrary.get_texture_parameter_names(mat)
    print(f'Texture param names: {list(info2)}')
    info3 = unreal.MaterialEditingLibrary.get_vector_parameter_names(mat)
    print(f'Vector param names: {list(info3)}')
