import unreal

# Clear the stale RelativeLocation override on FirstPersonCamera
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())

cam_comps = cdo.get_components_by_class(unreal.CameraComponent)
for cam in cam_comps:
    if cam.get_name() == 'FirstPersonCamera':
        old_loc = cam.get_editor_property('relative_location')
        print(f'BEFORE: RelativeLocation = {old_loc}')
        cam.set_editor_property('relative_location', unreal.Vector(0, 0, 0))
        new_loc = cam.get_editor_property('relative_location')
        print(f'AFTER:  RelativeLocation = {new_loc}')

# Save the BP
unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BP_GraceCharacter')
print('BP saved.')
