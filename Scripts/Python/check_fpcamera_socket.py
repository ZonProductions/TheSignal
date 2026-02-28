"""Check FPCamera socket transform on SKM_Operator_Mono."""
import unreal

skm = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if skm:
    socket = skm.find_socket('FPCamera')
    if socket:
        loc = socket.get_editor_property('relative_location')
        rot = socket.get_editor_property('relative_rotation')
        scale = socket.get_editor_property('relative_scale')
        bone = socket.get_editor_property('bone_name')
        print(f'FPCamera socket:')
        print(f'  Bone: {bone}')
        print(f'  Location: {loc}')
        print(f'  Rotation: {rot}')
        print(f'  Scale: {scale}')
    else:
        print('FPCamera socket not found')
