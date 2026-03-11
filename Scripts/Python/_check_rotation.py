import unreal
bp = unreal.load_asset("/Game/Core/Player/BP_GraceCharacter")
cdo = unreal.get_default_object(bp.generated_class())
print(f"bUseControllerRotationYaw = {cdo.get_editor_property('use_controller_rotation_yaw')}")
print(f"bUseControllerRotationPitch = {cdo.get_editor_property('use_controller_rotation_pitch')}")
print(f"bUseControllerRotationRoll = {cdo.get_editor_property('use_controller_rotation_roll')}")

# Check CMC
cmc = cdo.get_components_by_class(unreal.CharacterMovementComponent)
if cmc:
    c = cmc[0]
    print(f"bOrientRotationToMovement = {c.get_editor_property('orient_rotation_to_movement')}")
    print(f"bUseControllerDesiredRotation = {c.get_editor_property('use_controller_desired_rotation')}")
