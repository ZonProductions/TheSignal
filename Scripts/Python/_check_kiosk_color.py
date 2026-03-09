import unreal

# Check the MI_WCPackB material instance to get the actual door color
mi = unreal.load_asset('/Game/office_BigCompanyArchViz/Materials/MI_WCPackB')
if not mi:
    print('MI_WCPackB not found')
else:
    print(f'MI: {mi.get_name()} (class: {mi.get_class().get_name()})')

    # Scalar params
    scalars = mi.get_editor_property('scalar_parameter_values')
    for sp in scalars:
        pinfo = sp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = sp.get_editor_property('parameter_value')
        print(f'  Scalar: {pname} = {pval}')

    # Vector params
    vectors = mi.get_editor_property('vector_parameter_values')
    for vp in vectors:
        pinfo = vp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = vp.get_editor_property('parameter_value')
        print(f'  Vector: {pname} = (R={pval.r:.3f}, G={pval.g:.3f}, B={pval.b:.3f}, A={pval.a:.3f})')

    # Texture params
    textures = mi.get_editor_property('texture_parameter_values')
    for tp in textures:
        pinfo = tp.get_editor_property('parameter_info')
        pname = pinfo.get_editor_property('name')
        pval = tp.get_editor_property('parameter_value')
        print(f'  Texture: {pname} = {pval.get_name() if pval else "None"}')

    # Check parent material
    parent = mi.get_editor_property('parent')
    if parent:
        print(f'  Parent: {parent.get_name()} ({parent.get_path_name()})')
