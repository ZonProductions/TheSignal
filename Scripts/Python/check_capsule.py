"""Check capsule and eye height for BP_GraceTactical."""
import unreal

bp_class = unreal.load_object(None, '/Game/Player/Blueprints/BP_GraceTactical.BP_GraceTactical_C')
cdo = unreal.get_default_object(bp_class)

# Capsule
cap = cdo.capsule_component
if cap:
    hh = cap.get_editor_property('capsule_half_height')
    r = cap.get_editor_property('capsule_radius')
    print(f'Capsule HalfHeight: {hh}, Radius: {r}')

# BaseEyeHeight
beh = cdo.get_editor_property('base_eye_height')
print(f'BaseEyeHeight: {beh}')

# Check FPCamera socket world-space Z on the skeleton
skm = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if skm:
    socket = skm.find_socket('FPCamera')
    if socket:
        bone = socket.get_editor_property('bone_name')
        loc = socket.get_editor_property('relative_location')
        print(f'FPCamera socket: bone={bone}, relative_loc={loc}')
