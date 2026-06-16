# Wires the SignalSense waveform into WBP_HUD:
#  1) sets the SignalWaveMaterial class default to M_SignalWaveform (reliable)
#  2) best-effort: adds an Image named "SignalWave" to the root canvas so it binds + shows
# The C++ (UZP_HUDWidget) creates a DMI from SignalWaveMaterial and drives "Amplitude" each tick.
import unreal

WBP = "/Game/Blueprints/UI/WBP_HUD"
MAT = "/Game/UI/Signal/M_SignalWaveform"

wbp = unreal.load_asset(WBP)
mat = unreal.load_asset(MAT)
gen = wbp.generated_class()
cdo = unreal.get_default_object(gen)

# 1) class-default material
cdo.set_editor_property("signal_wave_material", mat)
unreal.log_warning("SIGNALWAVE_MAT_SET=%s" % str(cdo.get_editor_property("signal_wave_material")))

# 2) try to add the Image to the widget tree
added = False
try:
    tree = wbp.get_editor_property("widget_tree")
    existing = tree.find_widget_from_name("SignalWave")
    if existing:
        unreal.log_warning("SIGNALWAVE_IMG_EXISTS")
        added = True
    else:
        img = tree.construct_widget(unreal.Image, "SignalWave")
        img.set_brush_from_material(mat)
        root = tree.get_editor_property("root_widget")
        slot = root.add_child_to_canvas(img) if hasattr(root, "add_child_to_canvas") else None
        if slot:
            slot.set_size(unreal.Vector2D(420.0, 90.0))
            slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.5, 1.0), unreal.Vector2D(0.5, 1.0)))
            slot.set_alignment(unreal.Vector2D(0.5, 1.0))
            slot.set_position(unreal.Vector2D(0.0, -40.0))
        added = True
        unreal.log_warning("SIGNALWAVE_IMG_ADDED slot=%s" % ("ok" if slot else "no-canvas"))
except Exception as e:
    unreal.log_warning("SIGNALWAVE_IMG_FAILED: %s" % str(e))

# compile + save
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
except Exception as e:
    unreal.log_warning("COMPILE_SKIP: %s" % str(e))
unreal.EditorAssetLibrary.save_asset(WBP)
unreal.log_warning("WBP_SAVED added=%s" % str(added))
