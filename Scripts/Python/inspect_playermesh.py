"""Inspect PlayerMesh CDO state on BP_GraceCharacter."""
import unreal

bp = unreal.load_asset('/Game/Core/Player/BP_GraceCharacter')
cdo = unreal.get_default_object(bp.generated_class())

pm = cdo.get_editor_property('PlayerMesh')
if not pm:
    unreal.log_warning('[INSPECT] PlayerMesh is NULL on CDO!')
else:
    mesh_asset = pm.get_editor_property('SkeletalMeshAsset')
    loc = pm.get_editor_property('relative_location')
    rot = pm.get_editor_property('relative_rotation')
    anim_class = pm.get_editor_property('anim_class')
    visible = pm.get_editor_property('visible')
    hidden_in_game = pm.get_editor_property('bHiddenInGame')
    only_owner_see = pm.get_editor_property('only_owner_see')
    owner_no_see = pm.get_editor_property('owner_no_see')
    cast_shadow = pm.get_editor_property('cast_shadow')

    unreal.log(f'[INSPECT] SkeletalMeshAsset: {mesh_asset}')
    unreal.log(f'[INSPECT] Location: {loc}')
    unreal.log(f'[INSPECT] Rotation: {rot}')
    unreal.log(f'[INSPECT] AnimClass: {anim_class}')
    unreal.log(f'[INSPECT] Visible={visible} HiddenInGame={hidden_in_game} OnlyOwnerSee={only_owner_see} OwnerNoSee={owner_no_see} CastShadow={cast_shadow}')

# Also check inherited Mesh
body = cdo.get_editor_property('Mesh')
if body:
    body_mesh = body.get_editor_property('SkeletalMeshAsset')
    body_hidden = body.get_editor_property('bHiddenInGame')
    body_anim = body.get_editor_property('anim_class')
    unreal.log(f'[INSPECT] Inherited Mesh: asset={body_mesh} hidden={body_hidden} anim={body_anim}')
