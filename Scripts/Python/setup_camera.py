"""Set Camera1 socket to FPCamera and disable other cameras for GASP 5.4."""
import unreal

bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)

# Find all camera components
cameras = cdo.get_components_by_class(unreal.CameraComponent)
for cam in cameras:
    name = cam.get_name()
    print(f'Camera: {name}')
    if 'Camera1' in name:
        cam.set_editor_property('attach_socket_name', 'FPCamera')
        cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
        cam.set_editor_property('relative_rotation', unreal.Rotator(0, 0, 0))
        cam.set_editor_property('auto_activate', True)
        print(f'  -> Socket=FPCamera, transform reset, auto_activate=True')
    else:
        # GASP 5.4: disable other cameras
        cam.set_editor_property('auto_activate', False)
        print(f'  -> auto_activate=False')

bp = unreal.load_asset('/Game/Player/Blueprints/BP_GraceTactical')
unreal.EditorAssetLibrary.save_loaded_asset(bp)
print('Saved')
