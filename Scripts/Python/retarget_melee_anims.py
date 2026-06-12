"""
Retarget Kubold FPP Melee Animset (UE4 skeleton) onto the Operator mesh
(SKM_Operator_Mono, Kinemation SKM_Manny_Skeleton — UE5 bone names).

One body to rule them all: melee anims must drive the SAME shell the player
sees with every other weapon. This produces A_MeleePipe_* duplicates on the
Operator skeleton under /Game/TheSignal/Animations/Melee/.

Uses RTG_UE4Manny_UE5Manny (ships with LadderClimbingSystem pack).
Re-run safe: duplicate_and_retarget overwrites/duplicates as needed.

Session 62 (2026-06-11). Extend ANIMS list for new melee weapons.
"""
import unreal

SOURCE_MESH = '/Game/FPPMeleeAnimset/Models/UE4_Mannequin_NoHead/SK_MannequinNoHead'
TARGET_MESH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono'
RETARGETER  = '/Game/LadderClimbingSystem/Demo/Characters/Mannequin_UE4/Rigs/RTG_UE4Manny_UE5Manny'
ANIM_DIR    = '/Game/FPPMeleeAnimset/Animations/Longsword'
OUT_DIR     = '/Game/TheSignal/Animations/Melee'

# Longsword set (user-selected): light + heavy mix + transitions
ANIMS = [
    'FPP_Longs_Idle',
    'FPP_Longs_Equip',
    'FPP_Longs_Unequip',
    'FPP_Longs_Attack_F',        # light forward
    'FPP_Longs_Attack_R',        # light right
    'FPP_Longs_Attack_L',        # light left
    'FPP_Longs_AttackHeavy_F',   # heavy forward
    'FPP_Longs_AttackHeavy_R',   # heavy right
]

source = unreal.load_asset(SOURCE_MESH)
target = unreal.load_asset(TARGET_MESH)
rtg = unreal.load_asset(RETARGETER)
assert source and target and rtg, f'Missing: src={bool(source)} tgt={bool(target)} rtg={bool(rtg)}'

assets = []
for name in ANIMS:
    a = unreal.load_asset(f'{ANIM_DIR}/{name}')
    if a:
        # duplicate_and_retarget takes FAssetData, not UObject
        assets.append(unreal.AssetRegistryHelpers.create_asset_data(a))
    else:
        unreal.log_error(f'[retarget_melee] NOT FOUND: {name}')

unreal.log(f'[retarget_melee] Retargeting {len(assets)} anims...')

op = unreal.IKRetargetBatchOperation()
result = op.duplicate_and_retarget(
    assets_to_retarget=assets,
    source_mesh=source,
    target_mesh=target,
    ik_retarget_asset=rtg,
    search='FPP_Longs_',
    replace='A_MeleePipe_',
)
unreal.log(f'[retarget_melee] duplicate_and_retarget returned: {result}')

# Move results into OUT_DIR if they landed next to sources, then report
out = unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=True)
src_dir_new = [a for a in unreal.EditorAssetLibrary.list_assets(ANIM_DIR) if 'A_MeleePipe_' in a]
for a in src_dir_new:
    name = a.split('/')[-1].split('.')[0]
    unreal.EditorAssetLibrary.rename_asset(a, f'{OUT_DIR}/{name}')
out = unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=True)
unreal.log(f'[retarget_melee] Assets in {OUT_DIR}:')
for a in out:
    unreal.log(f'  {a}')
    unreal.EditorAssetLibrary.save_asset(a.split('.')[0])
unreal.log('[retarget_melee] DONE')
