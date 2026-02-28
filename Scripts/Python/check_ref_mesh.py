import unreal
# Check reference BP's inherited Mesh relative transform and socket
ref_bp = unreal.load_asset('/Game/KINEMATION/TacticalShooterPack/Blueprints/BP_TacticalShooterCharacter')
ref_cdo = unreal.get_default_object(ref_bp.generated_class())
ref_comps = ref_cdo.get_components_by_class(unreal.SkeletalMeshComponent)
for c in ref_comps:
    name = c.get_name()
    rel = c.get_editor_property('relative_location')
    rot = c.get_editor_property('relative_rotation')
    sk = c.get_editor_property('skeletal_mesh_asset')
    ac = c.get_editor_property('anim_class')
    print(f'{name}: RelLoc={rel}, RelRot={rot}, Mesh={sk}, AnimClass={ac}')
    if sk:
        fp = c.does_socket_exist('FPCamera')
        if fp:
            t = c.get_socket_transform('FPCamera', unreal.RelativeTransformSpace.RTS_COMPONENT)
            print(f'  FPCamera: Loc={t.translation}, Rot={t.rotation.rotator()}')

# Also check our BP
our_bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
our_cdo = unreal.get_default_object(our_bp.generated_class())
our_comps = our_cdo.get_components_by_class(unreal.SkeletalMeshComponent)
print('\n--- Our BP ---')
for c in our_comps:
    name = c.get_name()
    rel = c.get_editor_property('relative_location')
    rot = c.get_editor_property('relative_rotation')
    print(f'{name}: RelLoc={rel}, RelRot={rot}')
