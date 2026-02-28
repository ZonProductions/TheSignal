"""
Fix K002 (body clipping) + K004 (no shadow).

K002: Swap PlayerMesh from SKM_Operator_Mono (full body) to SK_Shirt_01a_FPP (arms-only).
      Reset Z offset to 0 (FPP mesh is designed for camera-level placement).
K004: Set inherited Mesh component to SKM_Operator_Mono as shadow-only body.

Run AFTER C++ rebuild (constructor sets shadow mesh properties).
"""
import unreal

eal = unreal.EditorAssetLibrary

BP_GRACE_PATH = '/Game/Core/Player/BP_GraceCharacter'
FPP_MESH_PATH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SK_Shirt_01a_FPP'
FULL_BODY_MESH_PATH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono'

# Load assets
bp_grace = eal.load_asset(BP_GRACE_PATH)
if not bp_grace:
    raise RuntimeError(f'Failed to load {BP_GRACE_PATH}')

fpp_mesh = eal.load_asset(FPP_MESH_PATH)
if not fpp_mesh:
    raise RuntimeError(f'Failed to load {FPP_MESH_PATH}')

full_body_mesh = eal.load_asset(FULL_BODY_MESH_PATH)
if not full_body_mesh:
    raise RuntimeError(f'Failed to load {FULL_BODY_MESH_PATH}')

gen_class = bp_grace.generated_class()
cdo = unreal.get_default_object(gen_class)

# ── K002: Swap PlayerMesh to FPP arms-only ────────────────────────
player_mesh = cdo.get_editor_property('PlayerMesh')
if player_mesh:
    player_mesh.set_editor_property('SkeletalMeshAsset', fpp_mesh)
    # Reset Z offset — FPP mesh doesn't need full-body offset
    player_mesh.set_editor_property('relative_location', unreal.Vector(0.0, 0.0, 0.0))
    unreal.log(f'[K002] PlayerMesh set to SK_Shirt_01a_FPP, Z offset reset to 0')
else:
    unreal.log_warning('[K002] PlayerMesh not found on CDO!')

# ── K004: Set inherited Mesh as shadow body ───────────────────────
body_mesh = cdo.get_editor_property('Mesh')
if body_mesh:
    body_mesh.set_editor_property('SkeletalMeshAsset', full_body_mesh)
    unreal.log(f'[K004] Inherited Mesh set to SKM_Operator_Mono (shadow body)')
else:
    unreal.log_warning('[K004] Inherited Mesh component not found on CDO!')

# ── Save ──────────────────────────────────────────────────────────
eal.save_asset(BP_GRACE_PATH)
unreal.log('[K002+K004] BP_GraceCharacter saved. FPP mesh + shadow body configured.')
