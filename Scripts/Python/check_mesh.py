import unreal
bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())
comps = cdo.get_components_by_class(unreal.SkeletalMeshComponent)
for c in comps:
    sk = c.get_editor_property('skeletal_mesh_asset')
    ac = c.get_editor_property('anim_class')
    print(f'{c.get_name()}: Mesh={sk}, AnimClass={ac}')
