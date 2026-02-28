"""Check if FPCamera socket exists on SKM_Operator_Mono."""
import unreal

skm = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if skm:
    # Use does_socket_exist
    for socket_name in ['FPCamera', 'fp_camera', 'head', 'Head', 'camera', 'Camera', 'VB head']:
        exists = skm.find_socket(socket_name)
        print(f'Socket "{socket_name}": {"EXISTS" if exists else "not found"}')

    # List all sockets on the mesh asset
    mesh_sockets = skm.get_editor_property('sockets')
    if mesh_sockets:
        print(f'Mesh sockets ({len(mesh_sockets)}):')
        for s in mesh_sockets:
            name = s.get_editor_property('socket_name')
            print(f'  {name}')
    else:
        print('No mesh-level sockets')
