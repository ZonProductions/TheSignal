"""Check if SKM_Operator_Mono has FPCamera socket."""
import unreal

mesh = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono')
if mesh:
    sockets = mesh.get_editor_property('sockets')
    if len(sockets) == 0:
        unreal.log('No sockets defined on mesh asset itself')
    else:
        for s in sockets:
            unreal.log(f'Mesh socket: {s.get_editor_property("socket_name")}')

    # Also check skeleton sockets
    skeleton = mesh.get_editor_property('skeleton')
    if skeleton:
        sk_sockets = skeleton.get_editor_property('sockets')
        found_fp = False
        for s in sk_sockets:
            name = str(s.get_editor_property('socket_name'))
            if 'Camera' in name or 'camera' in name or 'FP' in name:
                unreal.log(f'Skeleton socket (camera-related): {name} on bone {s.get_editor_property("bone_name")}')
                found_fp = True
        if not found_fp:
            unreal.log('No FPCamera socket found on skeleton')
        unreal.log(f'Total skeleton sockets: {len(sk_sockets)}')
else:
    unreal.log('SKM_Operator_Mono not found')

# Check UEFN mannequin too
uefn = unreal.load_asset('/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin')
if not uefn:
    # Try alternate paths
    for p in ['/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin',
              '/Game/Characters/Mannequins/Meshes/SKM_UEFN_Mannequin']:
        uefn = unreal.load_asset(p)
        if uefn:
            unreal.log(f'Found UEFN mannequin at: {p}')
            break
if not uefn:
    unreal.log('SKM_UEFN_Mannequin NOT FOUND in project')

# Check retargeter
rtg = unreal.load_asset('/Game/Characters/UEFN_Mannequin/Retarget/RTG_UEFN_to_UE5_Mannequin')
if not rtg:
    for p in ['/Game/gasp_Characters/UEFN_Mannequin/Retarget/RTG_UEFN_to_UE5_Mannequin',
              '/Game/KINEMATION/TacticalShooterPack/Retarget/RTG_UEFN_to_UE5_Mannequin']:
        rtg = unreal.load_asset(p)
        if rtg:
            unreal.log(f'Found retargeter at: {p}')
            break
if not rtg:
    unreal.log('RTG_UEFN_to_UE5_Mannequin NOT FOUND - need to locate or create')
