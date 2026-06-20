"""
Retarget the Kubold FPP Longsword BLOCK clips (UE4_Mannequin_Skeleton) onto the
Operator mesh (SKM_Operator_Mono / SKM_Manny_Skeleton, UE5). These were missed by
retarget_melee_anims.py, so they still play on the UE4 skeleton -> broken/elongated
hands during block while the (already-retargeted) idle is perfect.

Produces A_MeleePipe_Block* under /Game/TheSignal/Animations/Melee/.
"""
import unreal

SOURCE_MESH = '/Game/FPPMeleeAnimset/Models/UE4_Mannequin_NoHead/SK_MannequinNoHead'
TARGET_MESH = '/Game/KINEMATION/TacticalShooterPack/Character/Operator/UE5/SKM_Operator_Mono'
RTG_CANDIDATES = [
    '/Game/LadderClimbingSystem/Demo/Characters/Mannequin_UE4/Rigs/RTG_UE4Manny_UE5Manny',
    '/Game/AIBehaviorSystem/Demo/Characters/Mannequin_UE4/Rigs/RTG_UE4Manny_UE5Manny',
]
ANIM_DIR = '/Game/FPPMeleeAnimset/Animations/Longsword'
OUT_DIR  = '/Game/TheSignal/Animations/Melee'

ANIMS = [
    'FPP_Longs_BlockStart',
    'FPP_Longs_BlockLoop',
    'FPP_Longs_BlockWalk',
    'FPP_Longs_BlockStop',
    'FPP_Longs_BlockImpact1',
    'FPP_Longs_BlockImpact2',
    'FPP_Longs_BlockImpact3',
]

source = unreal.load_asset(SOURCE_MESH)
target = unreal.load_asset(TARGET_MESH)
rtg = None
for p in RTG_CANDIDATES:
    rtg = unreal.load_asset(p)
    if rtg:
        unreal.log('[retarget_block] using RTG: %s' % p); break
assert source and target and rtg, 'Missing: src=%s tgt=%s rtg=%s' % (bool(source), bool(target), bool(rtg))

# Clean up any earlier messy/partial retarget outputs so names come out clean.
for scan in ['/Game', OUT_DIR]:
    for a in unreal.EditorAssetLibrary.list_assets(scan, recursive=(scan == OUT_DIR)):
        if 'A_MeleePipe_Block' in a:
            unreal.EditorAssetLibrary.delete_asset(a.split('.')[0])

assets = []
for name in ANIMS:
    a = unreal.load_asset('%s/%s' % (ANIM_DIR, name))
    if a:
        assets.append(unreal.AssetRegistryHelpers.create_asset_data(a))
    else:
        unreal.log_error('[retarget_block] NOT FOUND: %s' % name)

unreal.log('[retarget_block] Retargeting %d anims...' % len(assets))
op = unreal.IKRetargetBatchOperation()
result = op.duplicate_and_retarget(
    assets_to_retarget=assets,
    source_mesh=source,
    target_mesh=target,
    ik_retarget_asset=rtg,
    search='FPP_Longs_',
    replace='A_MeleePipe_',
)
unreal.log('[retarget_block] returned: %s' % result)

# Results land at /Game/ root — move them into OUT_DIR.
for a in [x for x in unreal.EditorAssetLibrary.list_assets('/Game', recursive=False) if 'A_MeleePipe_Block' in x]:
    name = a.split('/')[-1].split('.')[0]
    unreal.EditorAssetLibrary.rename_asset(a.split('.')[0], '%s/%s' % (OUT_DIR, name))

ok = 0
for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=True):
    if 'A_MeleePipe_Block' in a:
        saved = unreal.EditorAssetLibrary.save_asset(a.split('.')[0])
        ld = unreal.load_asset(a.split('.')[0])
        skel = ld.get_editor_property('skeleton') if ld else None
        unreal.log('  %s  saved=%s skel=%s' % (a, saved, skel.get_name() if skel else None))
        ok += 1
unreal.log('[retarget_block] DONE (%d block clips)' % ok)
