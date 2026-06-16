# Rebuilds M_SignalWaveform as a CHRONOLOGICAL scope: the Custom node samples the
# component's amplitude-history texture (AmpHistory) per column, offset by WritePhase, so
# each bar is frozen at its captured height and scrolls left. Also re-points WBP_HUD.
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
pkg = "/Game/UI/Signal"
name = "M_SignalWaveform"
full = pkg + "/" + name
WBP = "/Game/Blueprints/UI/WBP_HUD"

HLSL = (
    "float N = 64.0;\n"
    "float bu = frac(WritePhase + UV.x);\n"
    "float bi = floor(bu * N);\n"
    "float bc = (bi + 0.5) / N;\n"
    "float a = Texture2DSample(AmpTex, AmpTexSampler, float2(bc, 0.5)).r;\n"
    "float MaxHeight = 0.25;\n"
    "float spike = frac(sin(bi*12.9898)*43758.5453);\n"
    "spike = spike*spike;\n"
    "float ampH = 0.10 + a*0.90;\n"
    "float h = (0.15 + spike*0.85) * ampH * MaxHeight;\n"
    "float gx = frac(bu * N);\n"
    "float gap = smoothstep(0.0,0.25,gx)*smoothstep(0.0,0.25,1.0-gx);\n"
    "float dist = abs(UV.y - 0.5);\n"
    "float bar = saturate((h - dist)*40.0);\n"
    "return bar*gap;"
)

wbp = unreal.load_asset(WBP)
cdo = unreal.get_default_object(wbp.generated_class())
cdo.set_editor_property("signal_wave_material", None)

aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(full):
    old = unreal.load_asset(full)
    aes.close_all_editors_for_asset(old)
    unreal.EditorAssetLibrary.delete_asset(full)

mat = tools.create_asset(name, pkg, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -600, 0)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
custom.set_editor_property("description", "SignalScope")
custom.set_editor_property("code", HLSL)

def ci(n):
    i = unreal.CustomInput()
    i.set_editor_property("input_name", n)
    return i
custom.set_editor_property("inputs", [ci("UV"), ci("Time"), ci("WritePhase"), ci("AmpTex")])

tc = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1000, -60)
tm = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -1000, 80)
wp = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, 200)
wp.set_editor_property("parameter_name", "WritePhase")
wp.set_editor_property("default_value", 0.0)
tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureObjectParameter, -1000, 340)
tex.set_editor_property("parameter_name", "AmpHistory")
deftex = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
if deftex:
    tex.set_editor_property("texture", deftex)

mel.connect_material_expressions(tc, "", custom, "UV")
mel.connect_material_expressions(tm, "", custom, "Time")
mel.connect_material_expressions(wp, "", custom, "WritePhase")
mel.connect_material_expressions(tex, "", custom, "AmpTex")

color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 260)
color.set_editor_property("parameter_name", "LineColor")
color.set_editor_property("default_value", unreal.LinearColor(0.15, 1.0, 0.55, 1.0))
mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 80)
mel.connect_material_expressions(color, "", mult, "A")
mel.connect_material_expressions(custom, "", mult, "B")

mel.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(full)

cdo.set_editor_property("signal_wave_material", mat)
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
except Exception as e:
    unreal.log_warning("compile skip: %s" % str(e))
unreal.EditorAssetLibrary.save_asset(WBP)
unreal.log_warning("SCOPE_MATERIAL_DONE hudRef=%s" % str(cdo.get_editor_property("signal_wave_material")))
