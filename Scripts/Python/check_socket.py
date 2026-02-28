import unreal
sk = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if sk:
    skel = sk.get_editor_property('skeleton')
    if skel:
        # Check for FPCamera socket
        names = unreal.SystemLibrary.get_display_name(skel)
        print(f'Skeleton: {names}')
        # Try to find socket by checking the mesh
        import unreal
        # Use SkeletalMesh API to get sockets
        sockets = sk.get_editor_property('sockets')
        print(f'Mesh sockets: {[s.get_editor_property("socket_name") for s in sockets]}')
    else:
        print('No skeleton found')
else:
    print('Could not load SKM_Operator_Mono')
