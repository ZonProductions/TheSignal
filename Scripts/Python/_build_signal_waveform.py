# Builds M_SignalWaveform — a scrolling oscilloscope-line UI material for the HUD.
# UI domain, animates off the Time node (no PIE needed to see motion in the material preview).
# Respects project lesson: UI-domain Custom HLSL must take TextureCoordinate as an INPUT,
# never Parameters.TexCoords[0].
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
pkg_path = "/Game/UI/Signal"
mat_name = "M_SignalWaveform"
mat_full = pkg_path + "/" + mat_name

aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(mat_full):
    _old = unreal.load_asset(mat_full)
    aes.close_all_editors_for_asset(_old)
    unreal.EditorAssetLibrary.delete_asset(mat_full)

mat = tools.create_asset(mat_name, pkg_path, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

# Custom HLSL node -> float1 line intensity
custom = mel.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 0)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
custom.set_editor_property("description", "SignalWaveform")
custom.set_editor_property("code",
    "float x = UV.x;\n"
    "float t = Time;\n"
    "float scroll = t*0.5;\n"
    "float N = 70.0;\n"
    "float col = floor((x + scroll)*N);\n"
    "float r = frac(sin(col*12.9898)*43758.5453);\n"
    "float r2 = frac(sin(col*4.1414 + 2.7)*1234.567);\n"
    "float spike = r*r*r2;\n"
    "float amp = 0.10 + Amp*0.90;\n"
    "float h = (0.12 + spike)*amp*0.5;\n"
    "float barX = frac((x + scroll)*N);\n"
    "float gap = smoothstep(0.0,0.15,barX)*smoothstep(0.0,0.15,1.0-barX);\n"
    "float dist = abs(UV.y - 0.5);\n"
    "float bar = saturate((h - dist)*50.0);\n"
    "return bar*gap;")

i_uv = unreal.CustomInput(); i_uv.set_editor_property("input_name", "UV")
i_t  = unreal.CustomInput(); i_t.set_editor_property("input_name", "Time")
i_a  = unreal.CustomInput(); i_a.set_editor_property("input_name", "Amp")
custom.set_editor_property("inputs", [i_uv, i_t, i_a])

tc = mel.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -850, -40)
tm = mel.create_material_expression(mat, unreal.MaterialExpressionTime, -850, 120)
amp = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -850, 240)
amp.set_editor_property("parameter_name", "Amplitude")
amp.set_editor_property("default_value", 0.7)

mel.connect_material_expressions(tc, "", custom, "UV")
mel.connect_material_expressions(tm, "", custom, "Time")
mel.connect_material_expressions(amp, "", custom, "Amp")

color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 220)
color.set_editor_property("parameter_name", "LineColor")
color.set_editor_property("default_value", unreal.LinearColor(0.15, 1.0, 0.55, 1.0))

mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 80)
mel.connect_material_expressions(color, "", mult, "A")
mel.connect_material_expressions(custom, "", mult, "B")

mel.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(custom, "", unreal.MaterialProperty.MP_OPACITY)

mel.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(mat_full)
unreal.log_warning("SIGNAL_WAVEFORM_MATERIAL_DONE: " + mat_full)
