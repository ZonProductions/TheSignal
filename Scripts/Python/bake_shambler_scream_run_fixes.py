"""
Two Shambler anim bakes (dev 2026-07-03), same technique as bake_shambler_run_arms.py
(duplicate -> overwrite bone-track keys with a frozen reference pose -> curve-strip -> save):

 1. A_Shambler_RunStiffArms -> A_Shambler_RunStiffArmsHead
    Also UN-ANIMATE the HEAD+NECK on the run ("the run looks goofy with the head swinging
    around — do the same thing with the head as you did with the arms"). Frozen at the same
    A_Shambler_IdleLL pose the arms use, so the whole upper body reads coherent.

 2. A_Shambler_Scream -> A_Shambler_ScreamPinned
    Freeze the LOWER BODY (hips/legs/feet/toes) at A_Shambler_Idle's stance ("the alert
    animation lifts the shambler's feet by a few inches — just the lower body, so the whole
    body doesn't move"). Upper body (spine up) still plays the full scream.

NEW assets only — never overwrites (asset-safety rule). UZP_ShamblerBehaviorComponent
LoadAnimDefaults points RunAnim/ScreamAnim at the new names after the C++ side lands.
Run via: POST :9847/api/python {"code": "exec(open('<this file>').read())"}
"""
import unreal

DIR = '/Game/Enemies/Shambler/Anims'
RUN_SRC = DIR + '/A_Shambler_RunStiffArms'
RUN_OUT = 'A_Shambler_RunStiffArmsHead'
SCREAM_SRC = DIR + '/A_Shambler_Scream'
SCREAM_OUT = 'A_Shambler_ScreamPinned'
IDLE_LL = DIR + '/A_Shambler_IdleLL'    # upper-body reference (arms bake used this)
IDLE_SHAM = DIR + '/A_Shambler_Idle'    # the shambler's own standing legs
POSE_TIME = 0.0

HEAD_TOKENS = ('neck', 'head')                            # run bake: freeze these
LOWER_TOKENS = ('hips', 'upleg', 'leg', 'foot', 'toe')    # scream bake: freeze these
# NOTE: 'leg' also matches 'upleg' (harmless double-match); arm bones (shoulder/arm/
# forearm/hand) contain none of these tokens, and 'spine' is deliberately NOT frozen.


def load(p):
    a = unreal.load_asset(p)
    if not a:
        raise RuntimeError('MISSING ASSET: %s' % p)
    return a


def bake(src_path, out_name, pose_asset_path, tokens, label):
    out_full = '%s/%s' % (DIR, out_name)
    if unreal.EditorAssetLibrary.does_asset_exist(out_full):
        unreal.log('[bake] SKIP (exists): %s' % out_full)
        return out_full
    if not unreal.EditorAssetLibrary.duplicate_asset(src_path, out_full):
        raise RuntimeError('duplicate_asset failed %s -> %s' % (src_path, out_full))

    pose_src = load(pose_asset_path)
    tgt = load(out_full)
    model = tgt.data_model_interface
    ctrl = tgt.controller
    track_names = [str(n) for n in model.get_bone_track_names()]
    targets = [n for n in track_names if any(t in n.lower() for t in tokens)]
    if not targets:
        raise RuntimeError('no tracks matched %s in %s (tracks: %s)' % (tokens, out_full, track_names[:10]))

    ctrl.open_bracket(label)
    frozen = 0
    try:
        for bone in targets:
            pose = unreal.AnimationLibrary.get_bone_pose_for_time(pose_src, bone, POSE_TIME, False)
            loc, rot, scale = pose.translation, pose.rotation, pose.scale3d
            if ctrl.set_bone_track_keys(bone, [loc, loc], [rot, rot], [scale, scale]):
                frozen += 1
            else:
                unreal.log_error('[bake] set_bone_track_keys FAILED for %s in %s' % (bone, out_full))
    finally:
        ctrl.close_bracket()

    curves = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
        tgt, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
    for c in curves:
        unreal.AnimationLibrary.remove_curve(tgt, c, False)
    saved = unreal.EditorAssetLibrary.save_asset(out_full)
    unreal.log('[bake] %s: %d/%d tracks frozen (%s @ t=%.2f), curves_stripped=%d saved=%s -> %s'
               % (label, frozen, len(targets), pose_asset_path.split('/')[-1], POSE_TIME, len(curves), saved, out_full))
    return out_full


bake(RUN_SRC, RUN_OUT, IDLE_LL, HEAD_TOKENS, 'Freeze run head+neck to LL idle')
bake(SCREAM_SRC, SCREAM_OUT, IDLE_SHAM, LOWER_TOKENS, 'Pin scream lower body to shambler idle')
