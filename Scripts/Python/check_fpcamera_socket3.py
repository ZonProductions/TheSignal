"""Check FPCamera socket on SKM_Operator_Mono skeleton."""
import unreal

mesh = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if mesh:
    skeleton = mesh.get_editor_property('skeleton')
    if skeleton:
        unreal.log(f'Skeleton: {skeleton.get_name()}')
        # Use num_sockets and get_socket_by_index
        num = skeleton.num_sockets()
        unreal.log(f'Total sockets: {num}')
        for i in range(num):
            sock = skeleton.get_socket_by_index(i)
            if sock:
                name = str(sock.get_editor_property('socket_name'))
                bone = str(sock.get_editor_property('bone_name'))
                if 'Camera' in name or 'camera' in name or 'FP' in name or 'fp' in name:
                    unreal.log(f'CAMERA SOCKET: {name} on bone {bone}')
