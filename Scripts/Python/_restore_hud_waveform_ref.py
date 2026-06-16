import unreal
WBP = "/Game/Blueprints/UI/WBP_HUD"
MAT = "/Game/UI/Signal/M_SignalWaveform"
wbp = unreal.load_asset(WBP)
mat = unreal.load_asset(MAT)
cdo = unreal.get_default_object(wbp.generated_class())
cdo.set_editor_property("signal_wave_material", mat)
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
except Exception as e:
    unreal.log_warning("compile skip: %s" % str(e))
unreal.EditorAssetLibrary.save_asset(WBP)
ref = cdo.get_editor_property("signal_wave_material")
unreal.log_warning("HUD_REF_RESTORED -> %s" % (ref.get_name() if ref else "NONE"))
