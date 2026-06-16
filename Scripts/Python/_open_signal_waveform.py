# Sets M_SignalWaveform preview to a flat plane and opens the material editor,
# so the scrolling line is viewable head-on (it animates off the Time node live).
import unreal

mat_full = "/Game/UI/Signal/M_SignalWaveform"
mat = unreal.load_asset(mat_full)

try:
    mat.set_editor_property("preview_mesh", unreal.SoftObjectPath("/Engine/BasicShapes/Plane.Plane"))
    unreal.EditorAssetLibrary.save_asset(mat_full)
    unreal.log_warning("PREVIEW_MESH_SET_PLANE")
except Exception as e:
    unreal.log_warning("PREVIEW_MESH_SET_FAILED (click the plane button manually): " + str(e))

aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
aes.open_editor_for_assets([mat])
unreal.log_warning("OPENED_MATERIAL_EDITOR")
