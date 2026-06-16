# Rebuild M_SignalWaveform with the new oscilloscope HLSL without leaving WBP_HUD with a
# broken reference: null the HUD's material ref, rebuild the material, re-point + recompile.
import unreal

WBP = "/Game/Blueprints/UI/WBP_HUD"
MAT = "/Game/UI/Signal/M_SignalWaveform"

wbp = unreal.load_asset(WBP)
cdo = unreal.get_default_object(wbp.generated_class())

# 1) drop the ref so the material can be deleted+recreated cleanly
cdo.set_editor_property("signal_wave_material", None)

# 2) rebuild the material (this script closes editors, deletes, recreates with new HLSL)
exec(open("C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_build_signal_waveform.py").read())

# 3) re-point the HUD at the fresh material, recompile + save
mat = unreal.load_asset(MAT)
cdo.set_editor_property("signal_wave_material", mat)
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
except Exception as e:
    unreal.log_warning("compile skip: %s" % str(e))
unreal.EditorAssetLibrary.save_asset(WBP)
unreal.log_warning("REWIRE_DONE mat=%s" % str(cdo.get_editor_property("signal_wave_material")))
