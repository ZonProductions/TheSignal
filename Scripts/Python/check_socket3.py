import unreal
# Use SkeletalMeshComponent to check socket at runtime
# Spawn a temp actor with the mesh and check
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
comps = cdo.get_components_by_class(unreal.SkeletalMeshComponent)
for c in comps:
    if c.get_name() == 'PlayerMesh':
        # Check if socket exists
        has_fp = c.does_socket_exist('FPCamera')
        print(f'PlayerMesh.does_socket_exist("FPCamera"): {has_fp}')
        # Also check common variations
        for name in ['FPCamera', 'fp_camera', 'FP_Camera', 'head', 'Head', 'camera']:
            print(f'  does_socket_exist("{name}"): {c.does_socket_exist(name)}')
        # List all socket names
        all_sockets = c.get_all_socket_names()
        print(f'All sockets ({len(all_sockets)}): {[str(s) for s in all_sockets[:20]]}')
        if len(all_sockets) > 20:
            print(f'  ... and {len(all_sockets) - 20} more')
