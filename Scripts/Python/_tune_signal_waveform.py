# Updates M_SignalWaveform's Custom HLSL in place (preserves all references):
#  - constant idle baseline jitter (never a dead flat line)
#  - denser/condensed waves + bright core + soft glow halo (real scope trace look)
import unreal

MAT = "/Game/UI/Signal/M_SignalWaveform"
mat = unreal.load_asset(MAT)

HLSL = (
    "float x = UV.x;\n"
    "float t = Time;\n"
    "// idle baseline jitter - always present so it never reads as a dead flat line\n"
    "float idle = sin(x*90.0 + t*6.0)*0.5 + sin(x*47.0 - t*3.7)*0.5;\n"
    "idle *= 0.05;\n"
    "// active signal - condensed (high freq), grows with Amp\n"
    "float sig = sin(x*120.0 + t*11.0)*0.55 + sin(x*61.0 - t*5.3)*0.45;\n"
    "sig *= Amp*0.40;\n"
    "float yv = 0.5 + idle + sig;\n"
    "float d = abs(UV.y - yv);\n"
    "// bright core + soft glow halo\n"
    "float core = saturate(1.0 - d*70.0);\n"
    "float halo = saturate(1.0 - d*12.0);\n"
    "return core + halo*0.4;"
)

# Find the Custom expression (UE5.7 keeps expressions on editor-only data).
exprs = None
try:
    eod = mat.get_editor_property("editor_only_data")
    coll = eod.get_editor_property("expression_collection")
    exprs = coll.get_editor_property("expressions")
except Exception as e:
    unreal.log_warning("EOD path failed: %s" % str(e))
if exprs is None:
    try:
        exprs = mat.get_editor_property("expressions")
    except Exception as e:
        unreal.log_warning("legacy expressions failed: %s" % str(e))

custom = None
for ex in (exprs or []):
    if isinstance(ex, unreal.MaterialExpressionCustom):
        custom = ex
        break

if custom:
    custom.set_editor_property("code", HLSL)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(MAT)
    unreal.log_warning("CUSTOM_CODE_UPDATED")
else:
    unreal.log_warning("CUSTOM_NODE_NOT_FOUND (exprs=%s)" % str(len(exprs) if exprs else 0))
