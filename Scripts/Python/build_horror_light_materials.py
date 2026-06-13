# Rebuilds the UE5.7 Horror-variant lighting materials natively in this UE5.4 project.
# Source recipe reverse-engineered from the unloadable 5.7 .uasset binaries.
# Assets created under /Game/HorrorLight/Materials.
import unreal

MEL = unreal.MaterialEditingLibrary
AT  = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary

FOLDER = "/Game/HorrorLight/Materials"

def make_asset(name, factory, cls):
    path = "{}/{}".format(FOLDER, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    a = AT.create_asset(name, FOLDER, cls, factory)
    print("  created", path, "->", a is not None)
    return a

def expr(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)

def func_expr(fn, cls, x, y):
    return MEL.create_material_expression_in_function(fn, cls, x, y)

# ---------------------------------------------------------------------------
# 1. MF_LightFlicker  (Material Function)
#    Flicker waveform floored by "Min Light" scalar param.
#    out = Min(1, MinLight + Saturate(CBS(Sine(Time))) * CBS(Sign(Sine(Time*17))))
# ---------------------------------------------------------------------------
def build_mf_flicker():
    print("MF_LightFlicker")
    fn = make_asset("MF_LightFlicker", unreal.MaterialFunctionFactoryNew(),
                    unreal.MaterialFunction)
    fn.set_editor_property("expose_to_library", True)

    minlight = func_expr(fn, unreal.MaterialExpressionScalarParameter, -900, 300)
    minlight.set_editor_property("parameter_name", "Min Light")
    minlight.set_editor_property("default_value", 0.3)
    minlight.set_editor_property("slider_max", 1.0)

    # smooth component
    time_s = func_expr(fn, unreal.MaterialExpressionTime, -900, -200)
    sine_s = func_expr(fn, unreal.MaterialExpressionSine, -700, -200)
    MEL.connect_material_expressions(time_s, "", sine_s, "")
    cbs_s = func_expr(fn, unreal.MaterialExpressionConstantBiasScale, -520, -200)
    cbs_s.set_editor_property("bias", 1.0); cbs_s.set_editor_property("scale", 0.5)
    MEL.connect_material_expressions(sine_s, "", cbs_s, "")
    sat = func_expr(fn, unreal.MaterialExpressionSaturate, -340, -200)
    MEL.connect_material_expressions(cbs_s, "", sat, "")

    # fast buzz component (square-ish via Sign)
    time_f = func_expr(fn, unreal.MaterialExpressionTime, -900, 50)
    mul_t = func_expr(fn, unreal.MaterialExpressionMultiply, -720, 50)
    mul_t.set_editor_property("const_b", 17.0)
    MEL.connect_material_expressions(time_f, "", mul_t, "A")
    sine_f = func_expr(fn, unreal.MaterialExpressionSine, -560, 50)
    MEL.connect_material_expressions(mul_t, "", sine_f, "")
    sign_f = func_expr(fn, unreal.MaterialExpressionSign, -400, 50)
    MEL.connect_material_expressions(sine_f, "", sign_f, "")
    cbs_f = func_expr(fn, unreal.MaterialExpressionConstantBiasScale, -240, 50)
    cbs_f.set_editor_property("bias", 1.0); cbs_f.set_editor_property("scale", 0.5)
    MEL.connect_material_expressions(sign_f, "", cbs_f, "")

    flick = func_expr(fn, unreal.MaterialExpressionMultiply, -100, -80)
    MEL.connect_material_expressions(sat, "", flick, "A")
    MEL.connect_material_expressions(cbs_f, "", flick, "B")

    add = func_expr(fn, unreal.MaterialExpressionAdd, 80, 100)
    MEL.connect_material_expressions(minlight, "", add, "A")
    MEL.connect_material_expressions(flick, "", add, "B")

    clamp = func_expr(fn, unreal.MaterialExpressionMin, 260, 100)
    clamp.set_editor_property("const_b", 1.0)
    MEL.connect_material_expressions(add, "", clamp, "A")

    out = func_expr(fn, unreal.MaterialExpressionFunctionOutput, 460, 100)
    out.set_editor_property("output_name", "Result")
    MEL.connect_material_expressions(clamp, "", out, "")

    MEL.update_material_function(fn)
    EAL.save_asset("{}/MF_LightFlicker".format(FOLDER))
    return fn

# ---------------------------------------------------------------------------
# 2. LF_Flicker  (Material, domain = Light Function)  -> inline flicker to Emissive
# ---------------------------------------------------------------------------
def build_lf_flicker():
    print("LF_Flicker")
    mat = make_asset("LF_Flicker", unreal.MaterialFactoryNew(), unreal.Material)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)

    minlight = expr(mat, unreal.MaterialExpressionScalarParameter, -900, 300)
    minlight.set_editor_property("parameter_name", "Min Light")
    minlight.set_editor_property("default_value", 0.3)
    minlight.set_editor_property("slider_max", 1.0)

    time_s = expr(mat, unreal.MaterialExpressionTime, -900, -200)
    sine_s = expr(mat, unreal.MaterialExpressionSine, -700, -200)
    MEL.connect_material_expressions(time_s, "", sine_s, "")
    cbs_s = expr(mat, unreal.MaterialExpressionConstantBiasScale, -520, -200)
    cbs_s.set_editor_property("bias", 1.0); cbs_s.set_editor_property("scale", 0.5)
    MEL.connect_material_expressions(sine_s, "", cbs_s, "")
    sat = expr(mat, unreal.MaterialExpressionSaturate, -340, -200)
    MEL.connect_material_expressions(cbs_s, "", sat, "")

    time_f = expr(mat, unreal.MaterialExpressionTime, -900, 50)
    mul_t = expr(mat, unreal.MaterialExpressionMultiply, -720, 50)
    mul_t.set_editor_property("const_b", 17.0)
    MEL.connect_material_expressions(time_f, "", mul_t, "A")
    sine_f = expr(mat, unreal.MaterialExpressionSine, -560, 50)
    MEL.connect_material_expressions(mul_t, "", sine_f, "")
    sign_f = expr(mat, unreal.MaterialExpressionSign, -400, 50)
    MEL.connect_material_expressions(sine_f, "", sign_f, "")
    cbs_f = expr(mat, unreal.MaterialExpressionConstantBiasScale, -240, 50)
    cbs_f.set_editor_property("bias", 1.0); cbs_f.set_editor_property("scale", 0.5)
    MEL.connect_material_expressions(sign_f, "", cbs_f, "")

    flick = expr(mat, unreal.MaterialExpressionMultiply, -100, -80)
    MEL.connect_material_expressions(sat, "", flick, "A")
    MEL.connect_material_expressions(cbs_f, "", flick, "B")
    add = expr(mat, unreal.MaterialExpressionAdd, 80, 100)
    MEL.connect_material_expressions(minlight, "", add, "A")
    MEL.connect_material_expressions(flick, "", add, "B")
    clamp = expr(mat, unreal.MaterialExpressionMin, 260, 100)
    clamp.set_editor_property("const_b", 1.0)
    MEL.connect_material_expressions(add, "", clamp, "A")

    MEL.connect_material_property(clamp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_asset("{}/LF_Flicker".format(FOLDER))
    return mat

# ---------------------------------------------------------------------------
# 3. M_Light  (Surface, Opaque)  -> emissive fixture material
#    EmissiveColor = Switch(UseLightFlicker, (LightCol*Strength*MF), (LightCol*Strength))
# ---------------------------------------------------------------------------
def build_m_light(mf):
    print("M_Light")
    mat = make_asset("M_Light", unreal.MaterialFactoryNew(), unreal.Material)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    lightcol = expr(mat, unreal.MaterialExpressionVectorParameter, -800, -150)
    lightcol.set_editor_property("parameter_name", "Light Col")
    lightcol.set_editor_property("default_value", unreal.LinearColor(1.0, 0.85, 0.6, 1.0))

    strength = expr(mat, unreal.MaterialExpressionScalarParameter, -800, 80)
    strength.set_editor_property("parameter_name", "Emissive Strength")
    strength.set_editor_property("default_value", 5.0)

    steady = expr(mat, unreal.MaterialExpressionMultiply, -560, -60)
    MEL.connect_material_expressions(lightcol, "", steady, "A")
    MEL.connect_material_expressions(strength, "", steady, "B")

    fcall = expr(mat, unreal.MaterialExpressionMaterialFunctionCall, -560, 200)
    fcall.set_editor_property("material_function", mf)

    flickered = expr(mat, unreal.MaterialExpressionMultiply, -340, 40)
    MEL.connect_material_expressions(steady, "", flickered, "A")
    MEL.connect_material_expressions(fcall, "Result", flickered, "B")

    sw = expr(mat, unreal.MaterialExpressionStaticSwitchParameter, -120, -20)
    sw.set_editor_property("parameter_name", "Use Light Flicker")
    sw.set_editor_property("default_value", True)
    MEL.connect_material_expressions(flickered, "", sw, "True")
    MEL.connect_material_expressions(steady, "", sw, "False")

    MEL.connect_material_property(sw, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_asset("{}/M_Light".format(FOLDER))
    return mat

# ---------------------------------------------------------------------------
# 4. MI_FlickeringLight  (instance of M_Light, flicker on)
# ---------------------------------------------------------------------------
def build_mi(parent):
    print("MI_FlickeringLight")
    mi = make_asset("MI_FlickeringLight", unreal.MaterialInstanceConstantFactoryNew(),
                    unreal.MaterialInstanceConstant)
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_static_switch_parameter_value(mi, "Use Light Flicker", True)
    EAL.save_asset("{}/MI_FlickeringLight".format(FOLDER))
    return mi

# ---------------------------------------------------------------------------
# 5. M_DustMote  (Translucent, Unlit)  -> ParticleColor emissive, radial opacity
# ---------------------------------------------------------------------------
def build_dustmote():
    print("M_DustMote")
    mat = make_asset("M_DustMote", unreal.MaterialFactoryNew(), unreal.Material)
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    pcolor = expr(mat, unreal.MaterialExpressionParticleColor, -500, -150)
    MEL.connect_material_property(pcolor, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    radial_fn = EAL.load_asset(
        "/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential")
    radial = expr(mat, unreal.MaterialExpressionMaterialFunctionCall, -500, 150)
    radial.set_editor_property("material_function", radial_fn)

    op = expr(mat, unreal.MaterialExpressionMultiply, -200, 100)
    MEL.connect_material_expressions(radial, "", op, "A")
    MEL.connect_material_expressions(pcolor, "A", op, "B")
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)

    MEL.recompile_material(mat)
    EAL.save_asset("{}/M_DustMote".format(FOLDER))
    return mat

print("=== Building Horror lighting materials in", FOLDER, "===")
mf = build_mf_flicker()
build_lf_flicker()
m_light = build_m_light(mf)
build_mi(m_light)
build_dustmote()
print("=== DONE ===")
