"""Shambler CRAWL anim set retarget (2026-08-05, crawling-stance feature).
NAAT zombie crawl clips -> necromorph, through the PROVEN pack pipeline: same
RTG_UE4_to_Shambler + BSP_ZombieAnims SK_Mannequin source mesh combination that produced
all six working grab clips (retarget_grab_anims.py BATCH A).
Post-steps per DEAD ENDS 2026-06-29 + 2026-08-04: strip float curves, clear additive
settings + force_root_lock on outputs, then SIZE-verify through the real evaluation path
(AnimPoseExtensions) against known-good A_Shambler_Hit_Front — never rotation-only."""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

NAAT_SRC = '/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
RTG_SHAM = '/Game/Enemies/Shambler/Rigs/RTG_UE4_to_Shambler'
OUT_DIR = '/Game/Enemies/Shambler/Anims'
GOOD_BASELINE = '/Game/Enemies/Shambler/Anims/A_Shambler_Hit_Front'

CLIPS = [
    ('/Game/BSP_ZombieAnims/Animations/Crawl/AS_NAAT_Zombie_Crawl_Idle', 'AS_NAAT_Zombie_Crawl_Idle', 'A_Shambler_CrawlIdle'),
    ('/Game/BSP_ZombieAnims/Animations/Crawl/AS_NAAT_Zombie_Crawl_Slow', 'AS_NAAT_Zombie_Crawl_Slow', 'A_Shambler_CrawlWalk'),
    ('/Game/BSP_ZombieAnims/Animations/Crawl/AS_NAAT_Zombie_FastCrawl', 'AS_NAAT_Zombie_FastCrawl', 'A_Shambler_CrawlRun'),
    ('/Game/BSP_ZombieAnims/Animations/Crawl/Attack/AS_NAAT_Zombie_Attack_LH_Crawl', 'AS_NAAT_Zombie_Attack_LH_Crawl', 'A_Shambler_Attack_Crawl'),
    # Stand attacks (dev 2026-08-05: part of the crawler set — it REARS UP to strike).
    ('/Game/BSP_ZombieAnims/Animations/Standing/Attack/AS_NAAT_Zombie_Attack_LH_Stand', 'AS_NAAT_Zombie_Attack_LH_Stand', 'A_Shambler_Attack_StandLH'),
    ('/Game/BSP_ZombieAnims/Animations/Standing/Attack/AS_NAAT_Zombie_Attack_BH_Stand', 'AS_NAAT_Zombie_Attack_BH_Stand', 'A_Shambler_Attack_StandBH'),
]


def load(p):
    a = unreal.load_asset(p)
    if not a:
        raise RuntimeError('MISSING ASSET: %s' % p)
    return a


def asset_data(path):
    name = path.split('/')[-1]
    ad = AR.get_asset_by_object_path('%s.%s' % (path, name))
    if str(ad.asset_class_path.asset_name) != 'AnimSequence':
        raise RuntimeError('bad class for %s' % path)
    return ad


def probe_size(path, label):
    """Evaluation-path size check: hips scale must be 1 and spine |T| ~927.8 (necromorph)."""
    a = load(path)
    ln = unreal.AnimationLibrary.get_sequence_length(a)
    opts = unreal.AnimPoseEvaluationOptions()
    ok = True
    for t in (0.0, ln * 0.5):
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(a, t, opts)
        hips = unreal.AnimPoseExtensions.get_bone_pose(pose, 'mixamorig_Hips_01', unreal.AnimPoseSpaces.LOCAL)
        spine = unreal.AnimPoseExtensions.get_bone_pose(pose, 'mixamorig_Spine_02', unreal.AnimPoseSpaces.LOCAL)
        sl = spine.translation.length()
        hs = hips.scale3d.x
        if abs(hs - 1.0) > 0.01 or abs(sl - 927.8) > 15.0:
            ok = False
        print('%s t=%.2f hipsScale=%.4f spine|T|=%.2f' % (label, t, hs, sl))
    return ok


rtg = load(RTG_SHAM)
src_mesh = load(NAAT_SRC)
tgt_mesh = load(NECRO_MESH)

all_ok = True
for src_path, src_name, out_name in CLIPS:
    out_full = '%s/%s' % (OUT_DIR, out_name)
    if unreal.EditorAssetLibrary.does_asset_exist(out_full):
        print('SKIP (exists): %s' % out_full)
        continue
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(src_path)], src_mesh, tgt_mesh, rtg,
        search=src_name, replace=out_name,
        target_path=OUT_DIR, overwrite_existing_files=False)
    if not new or len(new) == 0:
        raise RuntimeError('retarget produced NOTHING for %s' % src_path)
    if not unreal.EditorAssetLibrary.does_asset_exist(out_full):
        for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if src_name in base:
                unreal.EditorAssetLibrary.rename_asset(a.split('.')[0], out_full)
                break
    anim = load(out_full)
    # Orphan-curve strip (DEAD ENDS 2026-06-29 PostLoad crash class).
    curves = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
        anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
    for n in curves:
        unreal.AnimationLibrary.remove_curve(anim, n, False)
    # Additive/root-lock clear (DEAD ENDS 2026-08-04 — duplicate_and_retarget copies source flags).
    anim.set_editor_property('additive_anim_type', unreal.AdditiveAnimationType.AAT_NONE)
    anim.set_editor_property('ref_pose_type', unreal.AdditiveBasePoseType.ABPT_NONE)
    anim.set_editor_property('force_root_lock', False)
    saved = unreal.EditorAssetLibrary.save_asset(out_full)
    length = unreal.AnimationLibrary.get_sequence_length(anim)
    size_ok = probe_size(out_full, out_name)
    all_ok = all_ok and size_ok and saved
    print('OUT %s len=%.3f curves_stripped=%d saved=%s SIZE_%s' % (
        out_full, length, len(curves), saved, 'OK' if size_ok else 'FAIL'))

print('BASELINE reference:')
probe_size(GOOD_BASELINE, 'Hit_Front')
print('ALL_OK=%s' % all_ok)
