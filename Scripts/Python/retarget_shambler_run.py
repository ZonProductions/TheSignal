"""
Retarget AS_NAAT_Zombie_BL_Run -> A_Shambler_Run (necromorph skeleton).

Same proven pipeline as retarget_grab_anims.py BATCH A: the pack-authored
RTG_UE4_to_Shambler retargeter (UE4 mannequin -> necromorph), then curve-strip
(DEAD ENDS 2026-06-29: orphan curves crash UAnimSequence::PostLoad) and a
motion probe on mixamorig bones (necromorph probes on spine_01/pelvis names
silently return identity).

Idempotent: skips if /Game/Enemies/Shambler/Anims/A_Shambler_Run exists.
Run via: POST :9847/api/python {"code": "exec(open('<this file>').read())"}
"""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

NAAT_SRC = '/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
RTG_SHAM = '/Game/Enemies/Shambler/Rigs/RTG_UE4_to_Shambler'
SRC_ANIM = '/Game/BSP_ZombieAnims/Animations/Standing/AS_NAAT_Zombie_BL_Run'
OUT_DIR = '/Game/Enemies/Shambler/Anims'
OUT_NAME = 'A_Shambler_Run'
NECRO_BONES = ['mixamorig_spine_02', 'mixamorig_neck_05', 'mixamorig_hips_01']


def load(p):
    a = unreal.load_asset(p)
    if not a:
        raise RuntimeError('MISSING ASSET: %s' % p)
    return a


def asset_data(path):
    name = path.split('/')[-1]
    ad = AR.get_asset_by_object_path('%s.%s' % (path, name))
    cls = str(ad.asset_class_path.asset_name) if ad else 'None'
    if cls != 'AnimSequence':
        raise RuntimeError('AssetData class for %s is %s (want AnimSequence)' % (path, cls))
    return ad


out_full = '%s/%s' % (OUT_DIR, OUT_NAME)
if unreal.EditorAssetLibrary.does_asset_exist(out_full):
    unreal.log('[run-rtg] SKIP (exists): %s' % out_full)
else:
    rtg = load(RTG_SHAM)
    src_mesh = load(NAAT_SRC)
    tgt_mesh = load(NECRO_MESH)
    src_name = SRC_ANIM.split('/')[-1]
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(SRC_ANIM)], src_mesh, tgt_mesh, rtg,
        search=src_name, replace=OUT_NAME,
        target_path=OUT_DIR, overwrite_existing_files=True)
    if not new or len(new) == 0:
        raise RuntimeError('duplicate_and_retarget produced NOTHING for %s' % SRC_ANIM)
    if not unreal.EditorAssetLibrary.does_asset_exist(out_full):
        landed = None
        for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if src_name in base:
                landed = a.split('.')[0]
                break
        if landed:
            unreal.EditorAssetLibrary.rename_asset(landed, out_full)
        else:
            raise RuntimeError('output not found for %s in %s' % (SRC_ANIM, OUT_DIR))

# ---- strip curves + verify motion (always runs, even on SKIP) ----------------
anim = load(out_full)
skel = anim.get_editor_property('skeleton')
skel_path = skel.get_path_name() if skel else 'None'
if 'necromorph' not in skel_path:
    raise RuntimeError('%s wrong skeleton: %s' % (out_full, skel_path))
names = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
    anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
for n in names:
    unreal.AnimationLibrary.remove_curve(anim, n, False)
length = unreal.AnimationLibrary.get_sequence_length(anim)
moving = False
probe_used = None
for b in NECRO_BONES:
    try:
        e0 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, 0.0, False).rotation.euler()
        e1 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, length * 0.5, False).rotation.euler()
        e2 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, max(0.0, length - 0.05), False).rotation.euler()
        d = (e1 - e0).length() + (e2 - e1).length()
        probe_used = b
        if d > 1.0:
            moving = True
            break
    except Exception:
        continue
saved = unreal.EditorAssetLibrary.save_asset(out_full)
unreal.log('[run-rtg] %s len=%.3f curves_stripped=%d probe=%s moving=%s saved=%s skel=%s' % (
    out_full, length, len(names), probe_used, moving, saved, skel_path.split('/')[-1]))
if not moving:
    unreal.log_error('[run-rtg] STATIC OUTPUT: %s' % out_full)
if not saved:
    unreal.log_error('[run-rtg] SAVE FAILED (PIE running?): %s' % out_full)
