"""Disable GASP third-person camera and configure FPS camera on BP_GraceTactical."""
import unreal

bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)

# Find all camera components on the CDO
cameras = cdo.get_components_by_class(unreal.CameraComponent)
for cam in cameras:
    name = cam.get_name()
    print(f'Camera: {name} auto_activate={cam.get_editor_property("auto_activate")}')
    if 'Camera1' in name:
        # This is OUR FPS camera - keep it active
        cam.set_editor_property('auto_activate', True)
        print(f'  -> KEPT active (FPS camera)')
    else:
        # GASP third-person camera - disable it
        cam.set_editor_property('auto_activate', False)
        print(f'  -> DISABLED (GASP camera)')

# Find SpringArm and disable
spring_arms = cdo.get_components_by_class(unreal.SpringArmComponent)
for sa in spring_arms:
    name = sa.get_name()
    print(f'SpringArm: {name}')
    sa.set_editor_property('auto_activate', False)
    print(f'  -> DISABLED')

# Save
bp = unreal.load_asset('/Game/Player/Blueprints/BP_GraceTactical')
unreal.EditorAssetLibrary.save_loaded_asset(bp)
print('Saved BP_GraceTactical')
