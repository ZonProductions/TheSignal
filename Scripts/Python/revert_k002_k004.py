"""
Revert K002/K004 Blueprint changes — restore last-known-good state.

PlayerMesh: SKM_Operator_Mono at Z=-152 (was SK_Shirt_01a_FPP at Z=0)
Inherited Mesh: Clear SkeletalMeshAsset override (was SKM_Operator_Mono for shadow)

Run AFTER C++ rebuild (K004 shadow config removed from constructor).
"""
import unreal

eal = unreal.EditorAssetLibrary

BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'
OPERATOR_MONO_PATH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono'

# Load assets
bp_grace = eal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

operator_mono = eal.load_asset(OPERATOR_MONO_PATH)
if not operator_mono:
    raise RuntimeError(f'Failed to load {OPERATOR_MONO_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)

# ── Revert PlayerMesh to SKM_Operator_Mono at Z=-152 ──────────────
player_mesh = cdo.get_editor_property('PlayerMesh')
if player_mesh:
    player_mesh.set_editor_property('SkeletalMeshAsset', operator_mono)
    player_mesh.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, -152.0))
    player_mesh.set_editor_property('relative_rotation', unreal.Rotator(roll=0.0, pitch=0.0, yaw=-90.0))
    unreal.log('[REVERT] PlayerMesh -> SKM_Operator_Mono, Z=-152, Y=-90')
else:
    unreal.log_warning('[REVERT] PlayerMesh not found on CDO!')

# ── Clear inherited Mesh override (no shadow mesh without GASP) ───
body_mesh = cdo.get_editor_property('Mesh')
if body_mesh:
    body_mesh.set_editor_property('SkeletalMeshAsset', None)
    unreal.log('[REVERT] Inherited Mesh SkeletalMeshAsset cleared (no shadow mesh)')
else:
    unreal.log_warning('[REVERT] Inherited Mesh not found on CDO!')

# ── Save ──────────────────────────────────────────────────────────
eal.save_asset(BP_GRACE_PATH)
unreal.log('[REVERT] BP_GraceCharacter saved. K002/K004 reverted to last-known-good state.')
