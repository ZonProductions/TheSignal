import unreal
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
comps = cdo.get_components_by_class(unreal.SkeletalMeshComponent)
for c in comps:
    if c.get_name() == 'PlayerMesh':
        # Get socket transform in component space
        t = c.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_COMPONENT)
        print(f'FPCamera socket (component space):')
        print(f'  Location: {t.translation}')
        print(f'  Rotation: {t.rotation.rotator()}')
        # Also check head socket for reference
        t2 = c.get_socket_transform('head', unreal.RelativeTransformSpace.RTS_COMPONENT)
        print(f'head socket (component space):')
        print(f'  Location: {t2.translation}')
        print(f'  Rotation: {t2.rotation.rotator()}')
        # Check root bone
        t3 = c.get_socket_transform('root', unreal.RelativeTransformSpace.RTS_COMPONENT)
        print(f'root bone (component space):')
        print(f'  Location: {t3.translation}')
