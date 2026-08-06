"""Bake A_Shambler_StandUp — the crawler's rear-up transition (dev 2026-08-05: "try blending
the first 50% of AS_NAAT_Zombie_LieProne_To_Idle_Stand into AS_NAAT_Zombie_SlumpToStand").

Steps (idempotent):
 1. Retarget both GetUp sources -> A_Shambler_ProneToStand / A_Shambler_SlumpToStand
    (RTG_UE4_to_Shambler + NAAT SK_Mannequin, the proven pipeline; flags cleared, curves
    stripped, size-verified — the full 2026-08-04/05 checklist).
 2. Bake the composite: play ProneToStand 0..50%, crossfade (window W) into SlumpToStand
    from its start, then SlumpToStand up to ITS 50% ONLY (dev round-4: "get rid of the 2nd
    half of slump to stand, it's an entirely different position" — the tail floated mid-air).
    Rotation blend = hemisphere-corrected nlerp. 30fps via AnimDataController.
 3. Clear flags, strip curves, save, size-verify.
Re-bake: delete A_Shambler_StandUp and re-run (intermediates are skipped if present)."""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

NAAT_SRC = '/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
RTG_SHAM = '/Game/Enemies/Shambler/Rigs/RTG_UE4_to_Shambler'
OUT_DIR = '/Game/Enemies/Shambler/Anims'
FPS = 30.0
BLEND_MAX = 0.4  # crossfade window cap (s)

SOURCES = [
    ('/Game/BSP_ZombieAnims/Animations/GetUp/AS_NAAT_Zombie_LieProne_To_Idle_Stand', 'A_Shambler_ProneToStand'),
    ('/Game/BSP_ZombieAnims/Animations/GetUp/AS_NAAT_Zombie_SlumpToStand', 'A_Shambler_SlumpToStand'),
]
COMPOSITE = 'A_Shambler_StandUp'


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


def clean(anim):
    for n in [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
            anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]:
        unreal.AnimationLibrary.remove_curve(anim, n, False)
    anim.set_editor_property('additive_anim_type', unreal.AdditiveAnimationType.AAT_NONE)
    anim.set_editor_property('ref_pose_type', unreal.AdditiveBasePoseType.ABPT_NONE)
    anim.set_editor_property('force_root_lock', False)


def retarget(src_path, out_name):
    out_full = '%s/%s' % (OUT_DIR, out_name)
    if unreal.EditorAssetLibrary.does_asset_exist(out_full):
        print('SKIP retarget (exists): %s' % out_full)
        return out_full
    src_name = src_path.split('/')[-1]
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(src_path)], load(NAAT_SRC), load(NECRO_MESH), load(RTG_SHAM),
        search=src_name, replace=out_name, target_path=OUT_DIR, overwrite_existing_files=False)
    if not new or len(new) == 0:
        raise RuntimeError('retarget produced NOTHING for %s' % src_path)
    if not unreal.EditorAssetLibrary.does_asset_exist(out_full):
        for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if src_name in base:
                unreal.EditorAssetLibrary.rename_asset(a.split('.')[0], out_full)
                break
    anim = load(out_full)
    clean(anim)
    print('RETARGETED %s len=%.3f saved=%s' % (out_full, unreal.AnimationLibrary.get_sequence_length(anim),
                                               unreal.EditorAssetLibrary.save_asset(out_full)))
    return out_full


def qdot(a, b):
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w


def qnlerp(a, b, t):
    if qdot(a, b) < 0.0:
        b = unreal.Quat(-b.x, -b.y, -b.z, -b.w)
    q = unreal.Quat(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                    a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t)
    import math
    s = math.sqrt(qdot(q, q))
    if s < 1.e-8:
        return a
    return unreal.Quat(q.x / s, q.y / s, q.z / s, q.w / s)


def vlerp(a, b, t):
    return unreal.Vector(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t)


prone_p = retarget(*SOURCES[0])
slump_p = retarget(*SOURCES[1])
prone = load(prone_p)
slump = load(slump_p)

comp_full = '%s/%s' % (OUT_DIR, COMPOSITE)
if unreal.EditorAssetLibrary.does_asset_exist(comp_full):
    print('COMPOSITE already exists — delete %s and re-run to re-bake' % comp_full)
else:
    lenA = unreal.AnimationLibrary.get_sequence_length(prone)
    lenB_full = unreal.AnimationLibrary.get_sequence_length(slump)
    lenB = lenB_full * 0.5  # FIRST HALF ONLY — the tail is a different position (floats)
    halfA = lenA * 0.5
    W = min(BLEND_MAX, halfA * 0.5, lenB * 0.5)
    total = (halfA - W) + lenB
    n_frames = int(round(total * FPS))
    print('bake plan: lenA=%.3f halfA=%.3f lenB=%.3f blend=%.3f total=%.3f frames=%d' % (
        lenA, halfA, lenB, W, total, n_frames))

    if not unreal.EditorAssetLibrary.duplicate_asset(slump_p, comp_full):
        raise RuntimeError('duplicate_asset failed -> %s' % comp_full)
    comp = load(comp_full)
    model = comp.data_model_interface
    ctrl = comp.controller
    track_names = [str(n) for n in model.get_bone_track_names()]
    print('tracks=%d' % len(track_names))

    ctrl.open_bracket('Bake StandUp composite')
    try:
        try:
            ctrl.set_frame_rate(unreal.FrameRate(30, 1), False)
        except Exception as e:
            print('set_frame_rate: %s (continuing on source rate)' % e)
        resized = False
        for fn_name in ('set_number_of_frames', 'resize_number_of_frames'):
            try:
                getattr(ctrl, fn_name)(unreal.FrameNumber(n_frames), False)
                resized = True
                print('resized via %s -> %d frames' % (fn_name, n_frames))
                break
            except Exception as e:
                print('%s failed: %s' % (fn_name, e))
        if not resized:
            raise RuntimeError('could not resize composite — dir(ctrl): %s' %
                               [d for d in dir(ctrl) if 'frame' in d.lower() or 'number' in d.lower()])

        for bone in track_names:
            locs, rots, scales = [], [], []
            for f in range(n_frames + 1):
                t = min(f / FPS, total)
                if t < halfA - W:
                    p = unreal.AnimationLibrary.get_bone_pose_for_time(prone, bone, t, False)
                    loc, rot, sca = p.translation, p.rotation, p.scale3d
                elif t < halfA:
                    alpha = (t - (halfA - W)) / W
                    pa = unreal.AnimationLibrary.get_bone_pose_for_time(prone, bone, t, False)
                    pb = unreal.AnimationLibrary.get_bone_pose_for_time(slump, bone, t - (halfA - W), False)
                    loc = vlerp(pa.translation, pb.translation, alpha)
                    rot = qnlerp(pa.rotation, pb.rotation, alpha)
                    sca = vlerp(pa.scale3d, pb.scale3d, alpha)
                else:
                    p = unreal.AnimationLibrary.get_bone_pose_for_time(slump, bone, min(t - (halfA - W), lenB), False)
                    loc, rot, sca = p.translation, p.rotation, p.scale3d
                locs.append(loc)
                rots.append(rot)
                scales.append(sca)
            if not ctrl.set_bone_track_keys(bone, locs, rots, scales):
                print('set_bone_track_keys FAILED for %s' % bone)
    finally:
        ctrl.close_bracket()

    clean(comp)
    saved = unreal.EditorAssetLibrary.save_asset(comp_full)
    print('BAKED %s len=%.3f saved=%s' % (comp_full, unreal.AnimationLibrary.get_sequence_length(comp), saved))

# size-verify the composite start (prone plane) and end (standing plane)
comp = load(comp_full)
ln = unreal.AnimationLibrary.get_sequence_length(comp)
opts = unreal.AnimPoseEvaluationOptions()
try:
    opts.set_editor_property('optional_skeletal_mesh', load(NECRO_MESH))
except Exception:
    pass
for t in (0.0, ln * 0.5, ln * 0.98):
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(comp, t, opts)
    lowest, lb = None, ''
    for b in ('mixamorig_LeftHand_011', 'mixamorig_RightHand_019', 'mixamorig_LeftFoot_026',
              'mixamorig_RightFoot_030', 'mixamorig_LeftToeBase_027', 'mixamorig_RightToeBase_031',
              'mixamorig_LeftLeg_025', 'mixamorig_RightLeg_029', 'mixamorig_Head_06'):
        try:
            z = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation.z
            if lowest is None or z < lowest:
                lowest, lb = z, b
        except Exception:
            continue
    spine = unreal.AnimPoseExtensions.get_bone_pose(pose, 'mixamorig_Spine_02', unreal.AnimPoseSpaces.LOCAL)
    print('VERIFY t=%.2f lowest=%.0f (%s) UU=%.1f spine|T|=%.1f' % (t, lowest, lb, lowest * 0.01,
                                                                    spine.translation.length()))
