import unreal
sk = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if sk:
    skel = sk.get_editor_property('skeleton')
    if skel:
        # Check skeleton sockets
        skel_sockets = skel.get_editor_property('sockets')
        fp_found = False
        for s in skel_sockets:
            name = str(s.get_editor_property('socket_name'))
            if 'camera' in name.lower() or 'fp' in name.lower():
                print(f'FOUND: {name} on bone {s.get_editor_property("bone_name")}')
                fp_found = True
        if not fp_found:
            print('FPCamera socket NOT found on skeleton. All sockets:')
            for s in skel_sockets:
                print(f'  {s.get_editor_property("socket_name")} on {s.get_editor_property("bone_name")}')
