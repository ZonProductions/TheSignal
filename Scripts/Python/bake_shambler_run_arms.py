"""
Bake A_Shambler_RunStiffArms: the A_Shambler_Run body with UN-ANIMATED arms held
in the AS_NAAT_Zombie_LL_Idle arm pose (dev 2026-07-03: necromorphs aren't
traditional zombies — no arm swing on the sprint).

Steps (idempotent):
 1. Retarget AS_NAAT_Zombie_LL_Idle -> A_Shambler_IdleLL (RTG_UE4_to_Shambler,
    same pipeline as the run/grab clips) — only its ARM pose is used.
 2. Duplicate A_Shambler_Run -> A_Shambler_RunStiffArms.
 3. For every shoulder/arm/hand/finger bone track, overwrite the keys with the
    LL_Idle pose at ARM_POSE_TIME (constant keys) via AnimDataController.
 4. Curve-strip + save. UZP_ShamblerBehaviorComponent defaults RunAnim to this.

Re-bake with a different arm pose: change ARM_POSE_TIME, delete the RunStiffArms
asset, re-run. Run via: POST :9847/api/python {"code": "exec(open('<file>').read())"}
"""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

NAAT_SRC = '/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
RTG_SHAM = '/Game/Enemies/Shambler/Rigs/RTG_UE4_to_Shambler'
SRC_IDLE = '/Game/BSP_ZombieAnims/Animations/Hop/LeftLeg/AS_NAAT_Zombie_LL_Idle'
RUN_ANIM = '/Game/Enemies/Shambler/Anims/A_Shambler_Run'
OUT_DIR = '/Game/Enemies/Shambler/Anims'
IDLE_OUT = 'A_Shambler_IdleLL'
STIFF_OUT = 'A_Shambler_RunStiffArms'
ARM_POSE_TIME = 0.0  # seconds into A_Shambler_IdleLL to freeze the arms at

# lowercase substrings selecting the arm chains (shoulders down to fingertips)
ARM_TOKENS = ('shoulder', 'arm', 'hand', 'thumb', 'index', 'middle', 'ring', 'pinky')


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


# ---- 1. retarget the LL idle (arms source) -----------------------------------
idle_full = '%s/%s' % (OUT_DIR, IDLE_OUT)
if unreal.EditorAssetLibrary.does_asset_exist(idle_full):
    unreal.log('[stiff-arms] SKIP retarget (exists): %s' % idle_full)
else:
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(SRC_IDLE)], load(NAAT_SRC), load(NECRO_MESH), load(RTG_SHAM),
        search=SRC_IDLE.split('/')[-1], replace=IDLE_OUT,
        target_path=OUT_DIR, overwrite_existing_files=True)
    if not new or len(new) == 0:
        raise RuntimeError('retarget produced NOTHING for %s' % SRC_IDLE)
    if not unreal.EditorAssetLibrary.does_asset_exist(idle_full):
        for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if SRC_IDLE.split('/')[-1] in base:
                unreal.EditorAssetLibrary.rename_asset(a.split('.')[0], idle_full)
                break
    if not unreal.EditorAssetLibrary.does_asset_exist(idle_full):
        raise RuntimeError('retarget output not found: %s' % idle_full)

# ---- 2. duplicate the run ----------------------------------------------------
stiff_full = '%s/%s' % (OUT_DIR, STIFF_OUT)
if unreal.EditorAssetLibrary.does_asset_exist(stiff_full):
    unreal.log('[stiff-arms] SKIP duplicate (exists): %s' % stiff_full)
else:
    if not unreal.EditorAssetLibrary.duplicate_asset(RUN_ANIM, stiff_full):
        raise RuntimeError('duplicate_asset failed %s -> %s' % (RUN_ANIM, stiff_full))

idle = load(idle_full)
stiff = load(stiff_full)

# ---- 3. freeze the arm chains at the idle pose --------------------------------
# (data_model editor-property is None in 5.8 — the live interfaces are the python
#  attributes .data_model_interface / .controller)
model = stiff.data_model_interface
ctrl = stiff.controller
track_names = [str(n) for n in model.get_bone_track_names()]
targets = [n for n in track_names if any(t in n.lower() for t in ARM_TOKENS)]
if not targets:
    raise RuntimeError('no arm tracks matched in %s (tracks: %s)' % (stiff_full, track_names[:10]))

ctrl.open_bracket('Freeze arm chains to LL idle pose')
frozen = 0
try:
    for bone in targets:
        pose = unreal.AnimationLibrary.get_bone_pose_for_time(idle, bone, ARM_POSE_TIME, False)
        loc = pose.translation
        rot = pose.rotation
        scale = pose.scale3d
        ok = ctrl.set_bone_track_keys(bone, [loc, loc], [rot, rot], [scale, scale])
        if not ok:
            unreal.log_error('[stiff-arms] set_bone_track_keys FAILED for %s' % bone)
        else:
            frozen += 1
finally:
    ctrl.close_bracket()

# ---- 4. strip curves + save ----------------------------------------------------
names = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
    stiff, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
for n in names:
    unreal.AnimationLibrary.remove_curve(stiff, n, False)
saved_i = unreal.EditorAssetLibrary.save_asset(idle_full)
saved_s = unreal.EditorAssetLibrary.save_asset(stiff_full)
unreal.log('[stiff-arms] DONE %s: %d/%d arm tracks frozen (pose t=%.2fs of %s), curves_stripped=%d, saved idle=%s stiff=%s'
           % (stiff_full, frozen, len(targets), ARM_POSE_TIME, IDLE_OUT, len(names), saved_i, saved_s))
