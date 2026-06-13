import unreal
MEL = unreal.MaterialEditingLibrary
F = "/Game/HorrorLight/Materials"
for n in ["MF_LightFlicker","LF_Flicker","M_Light","MI_FlickeringLight","M_DustMote"]:
    a = unreal.load_asset("{}/{}".format(F,n))
    if a is None:
        print(n, "LOAD FAIL"); continue
    if isinstance(a, unreal.MaterialFunction):
        print(n, "MaterialFunction OK, exprs=", MEL.get_num_material_expressions_in_function(a) if hasattr(MEL,'get_num_material_expressions_in_function') else '?')
    elif isinstance(a, unreal.MaterialInstanceConstant):
        print(n, "MIC parent=", a.get_editor_property("parent").get_name() if a.get_editor_property("parent") else None)
    elif isinstance(a, unreal.Material):
        print(n, "domain=", a.get_editor_property("material_domain"), "blend=", a.get_editor_property("blend_mode"), "exprs=", MEL.get_num_material_expressions(a))
