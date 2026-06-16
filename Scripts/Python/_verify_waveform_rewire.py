import unreal
mat = unreal.load_asset("/Game/UI/Signal/M_SignalWaveform")
wbp = unreal.load_asset("/Game/Blueprints/UI/WBP_HUD")
cdo = unreal.get_default_object(wbp.generated_class())
ref = cdo.get_editor_property("signal_wave_material")
unreal.log_warning("WAVE_VERIFY matExists=%s domain=%s hudRef=%s" % (
    mat is not None,
    str(mat.get_editor_property("material_domain")) if mat else "-",
    ref.get_name() if ref else "NONE"))
