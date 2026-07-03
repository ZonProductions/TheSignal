"""
Grab/Struggle system retarget batch (Docs/Plan_GrabStruggle.md section 12 step 1).
REVISED 2026-07-02 (v2): the original Python-created RTG_UE4_to_UEFN DOUBLED neck rotation
("top of head facing the floor") — the UE4 rig has ONE combined chain Head[neck_01->Head],
the UE5-family rigs have TWO (Neck[neck_01->neck_02] + Head[Head->Head]), and auto-mapping
fed the full source rotation into BOTH target chains. RULE: cross the UE4->UE5 boundary ONLY
via the pack's human-authored RTG_UE4Manny_UE5Manny; Python-created retargeters are safe only
between SAME-topology UE5-family skeletons (Manny/UEFN/CC), where auto-mapping is 1:1.

Produces:
  /Game/Enemies/Shambler/Anims/A_Shambler_Grab*   (6 clips, necromorph skeleton)
  /Game/Marcus/GrabAnims/A_Marcus_Grab*           (5 clips, CCMH skeleton - MarcusBody)
  /Game/Marcus/GrabAnims/A_Marcus_KnockdownFP     (UEFN skeleton - hidden loco Mesh)
  /Game/Marcus/GrabAnims/A_Marcus_GetUpBack       (UEFN skeleton - hidden loco Mesh)
Creates (idempotent): /Game/Marcus/Rigs/RTG_UE5Manny_to_UEFN, /Game/Marcus/Rigs/RTG_CC_to_UEFN.
DO NOT USE /Game/Marcus/Rigs/RTG_UE4_to_UEFN (broken neck mapping — deleted in v2 run).

Every output is curve-stripped (DEAD ENDS 2026-06-29) and motion+neck verified.
"""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

# ---- meshes -----------------------------------------------------------------
NAAT_SRC = '/Game/BSP_ZombieAnims/EpicAssets/Mannequin/Character/Mesh/SK_Mannequin'
FPP_SRC = '/Game/FPPMeleeAnimset/UE4_Mannequin/Mesh/SK_Mannequin'
CC_SRC = '/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Dummy/CC_Dummy'
MANNY_MESH = '/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Meshes/SKM_Manny'
UEFN_MESH = '/Game/gasp_Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
CCMH_MESH = '/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male'

# ---- retargeters ------------------------------------------------------------
RTG_SHAM = '/Game/Enemies/Shambler/Rigs/RTG_UE4_to_Shambler'   # FPP-UE4 -> necromorph (pack-era, proven)
RTG_UE4_MANNY = '/Game/AIBehaviorSystem/Demo/Characters/Mannequin_UE4/Rigs/RTG_UE4Manny_UE5Manny'  # PROVEN UE4->UE5 crossing
RTG_MH = '/Game/CharacterCustomizer/Characters/CCMH/Animation/MH_Retargeter'  # UE5-family -> CCMH (proven)
IK_MANNY = '/Game/AIBehaviorSystem/Demo/Characters/Mannequins/Rigs/IK_Mannequin'
IK_UEFN = '/Game/gasp_Characters/UEFN_Mannequin/Rigs/IK_UEFN_Mannequin'
IK_MH = '/Game/CharacterCustomizer/Characters/CCMH/Animation/MH_IK_Rig'

RIGS_DIR = '/Game/Marcus/Rigs'
SHAM_OUT = '/Game/Enemies/Shambler/Anims'
MARCUS_OUT = '/Game/Marcus/GrabAnims'
TMP_OUT = '/Game/Marcus/GrabAnims/_Intermediate'

NAAT_Z = '/Game/BSP_ZombieAnims/Animations/Interactive/Zombie'
NAAT_H = '/Game/BSP_ZombieAnims/Animations/Interactive/Human'

results = []


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


def get_or_create_rtg(pkg_path, name, ik_src_path, ik_tgt_path, src_mesh_path, tgt_mesh_path):
    full = '%s/%s' % (pkg_path, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.log('[grab-rtg] reusing existing %s' % full)
        return load(full)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rtg = tools.create_asset(name, pkg_path, unreal.IKRetargeter, unreal.IKRetargetFactory())
    if not rtg:
        raise RuntimeError('create_asset failed for %s' % full)
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, load(ik_src_path))
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, load(ik_tgt_path))
    try:
        rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, load(src_mesh_path))
        rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, load(tgt_mesh_path))
    except Exception as e:
        unreal.log_warning('[grab-rtg] set_preview_mesh: %s (non-fatal)' % e)
    # UE5.8: factory-created retargeters have an EMPTY op stack -> STATIC output. Add the
    # default ops FIRST, then run initial setup per op (fills chain mappings from the rigs).
    if rc.get_num_retarget_ops() == 0:
        rc.add_default_ops()
    n = rc.get_num_retarget_ops()
    for i in range(n):
        rc.run_op_initial_setup(i)
    unreal.log('[grab-rtg] created %s (%d ops)' % (full, n))
    unreal.EditorAssetLibrary.save_asset(full)
    return rtg


def get_or_create_split_ccmh_rig():
    """IK rig for CCMH with SPLIT Neck/Head chains, chain names mirroring IK_UEFN_Mannequin
    exactly (auto-map pairs by name -> guaranteed 1:1). Needed because MH_IK_Rig evidently
    uses a combined neck chain (a CCMH->UEFN hop through it doubled the neck: 19/43/54 vs
    the correct 19/19/29). FK-only retargets need chains + retarget root, no solvers."""
    full = '%s/IK_CCMH_Split' % RIGS_DIR
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return load(full)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rig = tools.create_asset('IK_CCMH_Split', RIGS_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    rc = unreal.IKRigController.get_controller(rig)
    rc.set_skeletal_mesh(load(CCMH_MESH))
    rc.set_retarget_root('pelvis')
    CHAINS = [
        ('Root', 'root', 'root'),
        ('Spine', 'spine_01', 'spine_05'),
        ('Neck', 'neck_01', 'neck_02'),
        ('Head', 'head', 'head'),
        ('LeftClavicle', 'clavicle_l', 'clavicle_l'),
        ('RightClavicle', 'clavicle_r', 'clavicle_r'),
        ('LeftArm', 'upperarm_l', 'hand_l'),
        ('RightArm', 'upperarm_r', 'hand_r'),
        ('LeftLeg', 'thigh_l', 'foot_l'),
        ('RightLeg', 'thigh_r', 'foot_r'),
        ('LeftToe', 'ball_l', 'ball_l'),
        ('RightToe', 'ball_r', 'ball_r'),
        ('LeftThumb', 'thumb_01_l', 'thumb_03_l'),
        ('LeftIndex', 'index_01_l', 'index_03_l'),
        ('LeftMiddle', 'middle_01_l', 'middle_03_l'),
        ('LeftRing', 'ring_01_l', 'ring_03_l'),
        ('LeftPinky', 'pinky_01_l', 'pinky_03_l'),
        ('RightThumb', 'thumb_01_r', 'thumb_03_r'),
        ('RightIndex', 'index_01_r', 'index_03_r'),
        ('RightMiddle', 'middle_01_r', 'middle_03_r'),
        ('RightRing', 'ring_01_r', 'ring_03_r'),
        ('RightPinky', 'pinky_01_r', 'pinky_03_r'),
    ]
    ok = 0
    for name, sb, eb in CHAINS:
        try:
            rc.add_retarget_chain(name, sb, eb, 'None')
            ok += 1
        except Exception as e:
            unreal.log_warning('[grab-rtg] chain %s(%s->%s) failed: %s' % (name, sb, eb, e))
    unreal.log('[grab-rtg] created IK_CCMH_Split (%d/%d chains)' % (ok, len(CHAINS)))
    unreal.EditorAssetLibrary.save_asset(full)
    return rig


def retarget_one(src_anim_path, rtg, src_mesh, tgt_mesh, out_dir, out_name):
    src_name = src_anim_path.split('/')[-1]
    out_full = '%s/%s' % (out_dir, out_name)
    if unreal.EditorAssetLibrary.does_asset_exist(out_full):
        unreal.log('[grab-rtg] SKIP (exists): %s' % out_full)
        return out_full
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(src_anim_path)], src_mesh, tgt_mesh, rtg,
        search=src_name, replace=out_name,
        target_path=out_dir, overwrite_existing_files=True)
    if not new or len(new) == 0:
        raise RuntimeError('duplicate_and_retarget produced NOTHING for %s' % src_anim_path)
    if not unreal.EditorAssetLibrary.does_asset_exist(out_full):
        landed = None
        for a in unreal.EditorAssetLibrary.list_assets(out_dir, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if src_name in base:
                landed = a.split('.')[0]
                break
        if landed:
            unreal.EditorAssetLibrary.rename_asset(landed, out_full)
        else:
            raise RuntimeError('output not found for %s in %s' % (src_anim_path, out_dir))
    return out_full


def strip_curves_and_verify(path, expect_skel_substr, probe_bones, neck_check=None):
    anim = load(path)
    skel = anim.get_editor_property('skeleton')
    skel_path = skel.get_path_name() if skel else 'None'
    if expect_skel_substr not in skel_path:
        raise RuntimeError('%s wrong skeleton: %s' % (path, skel_path))
    names = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
        anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
    for n in names:
        unreal.AnimationLibrary.remove_curve(anim, n, False)
    length = unreal.AnimationLibrary.get_sequence_length(anim)
    moving = False
    probe_used = None
    for b in probe_bones:
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
    # Neck sanity: total |neck_02|+|head| local rotation must not DOUBLE the source's head
    # rotation (the RTG_UE4_to_UEFN failure mode). neck_check = source total to compare against.
    neck_note = ''
    if neck_check is not None:
        tot = 0.0
        for b in ('neck_02', 'head'):
            try:
                e = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, length * 0.5, False).rotation.euler()
                tot += e.length()
            except Exception:
                pass
        neck_note = ' neckTot=%.0f(src %.0f)' % (tot, neck_check)
        if neck_check > 10.0 and tot > neck_check * 1.6:
            unreal.log_error('[grab-rtg] NECK DOUBLING suspected on %s (%.0f vs src %.0f)' % (path, tot, neck_check))
    saved = unreal.EditorAssetLibrary.save_asset(path)
    line = '%s len=%.3f curves_stripped=%d probe=%s moving=%s saved=%s%s skel=%s' % (
        path, length, len(names), probe_used, moving, saved, neck_note, skel_path.split('/')[-1])
    results.append(line)
    unreal.log('[grab-rtg] %s' % line)
    if not moving:
        unreal.log_error('[grab-rtg] STATIC OUTPUT (empty op stack?): %s' % path)
    if not saved:
        unreal.log_error('[grab-rtg] SAVE FAILED (PIE running?): %s' % path)


def src_head_total(src_path):
    anim = load(src_path)
    L = unreal.AnimationLibrary.get_sequence_length(anim)
    tot = 0.0
    for b in ('neck_01', 'head'):
        try:
            e = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, L * 0.5, False).rotation.euler()
            tot += e.length()
        except Exception:
            pass
    return tot


# =============================================================================
unreal.log('[grab-rtg] ===== BATCH A: NAAT Zombie -> necromorph (Shambler) =====')
rtg_sham = load(RTG_SHAM)
naat_src = load(NAAT_SRC)
necro = load(NECRO_MESH)
SHAM_MAP = [
    ('AS_NAAT_Zombie_Idle_To_Grab', 'A_Shambler_GrabEntry'),
    ('AS_NAAT_Zombie_Grab_To_Munching', 'A_Shambler_GrabMunch'),
    ('AS_NAAT_Zombie_Grab_To_Wrestle', 'A_Shambler_GrabWrestle'),
    ('AS_NAAT_Zombie_Grab_To_Kicked', 'A_Shambler_GrabKicked'),
    ('AS_NAAT_Zombie_Grab_To_Pushed', 'A_Shambler_GrabPushed'),
    ('AS_NAAT_Zombie_Grab_To_TakeDown', 'A_Shambler_GrabTakedown'),
]
sham_outs = []
for src, dst in SHAM_MAP:
    sham_outs.append(retarget_one('%s/%s' % (NAAT_Z, src), rtg_sham, naat_src, necro, SHAM_OUT, dst))

unreal.log('[grab-rtg] ===== BATCH B: NAAT Human -> UE5Manny (PROVEN pack RTG) -> CCMH =====')
rtg_ue4_manny = load(RTG_UE4_MANNY)
manny_mesh = load(MANNY_MESH)
ccmh = load(CCMH_MESH)
rtg_mh = load(RTG_MH)
HUMAN_MAP = [
    ('AS_NAAT_Human_Idle_To_Grab', 'A_Marcus_GrabEntry'),
    ('AS_NAAT_Human_Grab_To_Munching', 'A_Marcus_GrabMunch'),
    ('AS_NAAT_Human_Grab_To_Wrestle', 'A_Marcus_GrabWrestle'),
    ('AS_NAAT_Human_Grab_To_Kick', 'A_Marcus_GrabKick'),
    ('AS_NAAT_Human_Grab_To_Push', 'A_Marcus_GrabPush'),
]
marcus_outs = []
for src, dst in HUMAN_MAP:
    tmp = retarget_one('%s/%s' % (NAAT_H, src), rtg_ue4_manny, naat_src, manny_mesh, TMP_OUT, 'TMP_%s' % dst)
    marcus_outs.append((retarget_one(tmp, rtg_mh, manny_mesh, ccmh, MARCUS_OUT, dst),
                        src_head_total('%s/%s' % (NAAT_H, src))))

unreal.log('[grab-rtg] ===== BATCH C: FPP knockdown -> UE5Manny (pack RTG) -> UEFN =====')
rtg_manny_uefn = get_or_create_rtg(RIGS_DIR, 'RTG_UE5Manny_to_UEFN', IK_MANNY, IK_UEFN, MANNY_MESH, UEFN_MESH)
fpp_src = load(FPP_SRC)
uefn_mesh = load(UEFN_MESH)
kd_tmp = retarget_one('/Game/FPPMeleeAnimset/Animations/Dagger/FPP_Dag_Knockdown',
                      rtg_ue4_manny, fpp_src, manny_mesh, TMP_OUT, 'TMP_A_Marcus_KnockdownFP')
kd_out = retarget_one(kd_tmp, rtg_manny_uefn, manny_mesh, uefn_mesh, MARCUS_OUT, 'A_Marcus_KnockdownFP')
kd_src_tot = src_head_total('/Game/FPPMeleeAnimset/Animations/Dagger/FPP_Dag_Knockdown')

unreal.log('[grab-rtg] ===== BATCH D: Get_Up_Back (CC) -> CCMH (PROVEN MH_Retargeter) -> UEFN =====')
# The CC rig has the combined-neck topology (Python RTG_CC_to_UEFN doubled the neck — deleted),
# and so does MH_IK_Rig (a CCMH->UEFN hop through it doubled again). Cross CC via the pack's
# MH_Retargeter, then hop CCMH->UEFN via a custom SPLIT-chain CCMH rig (names mirror IK_UEFN).
ik_ccmh_split = get_or_create_split_ccmh_rig()
rtg_ccmh_uefn = get_or_create_rtg(RIGS_DIR, 'RTG_CCMH_to_UEFN',
                                  '%s/IK_CCMH_Split' % RIGS_DIR, IK_UEFN, CCMH_MESH, UEFN_MESH)
cc_src = load(CC_SRC)
gu_tmp = retarget_one('/Game/CharacterCustomizer/Components/SLS/Animations/Ragdoll/Get_Up_Back',
                      rtg_mh, cc_src, ccmh, TMP_OUT, 'TMP_A_Marcus_GetUpBack')
gu_out = retarget_one(gu_tmp, rtg_ccmh_uefn, ccmh, uefn_mesh, MARCUS_OUT, 'A_Marcus_GetUpBack')
gu_src_tot = src_head_total('/Game/CharacterCustomizer/Components/SLS/Animations/Ragdoll/Get_Up_Back')

unreal.log('[grab-rtg] ===== CURVE AUDIT + MOTION/NECK VERIFY =====')
NECRO_BONES = ['mixamorig_spine_02', 'mixamorig_neck_05', 'mixamorig_hips_01']
UE5_BONES = ['spine_02', 'spine_01', 'pelvis']
for p in sham_outs:
    strip_curves_and_verify(p, 'necromorph', NECRO_BONES)
for p, src_tot in marcus_outs:
    strip_curves_and_verify(p, 'CCMH_Skeleton', UE5_BONES, neck_check=src_tot)
strip_curves_and_verify(kd_out, 'UEFN', UE5_BONES, neck_check=kd_src_tot)
strip_curves_and_verify(gu_out, 'UEFN', UE5_BONES, neck_check=gu_src_tot)

unreal.log('[grab-rtg] ===== DONE: %d outputs =====' % len(results))
for r in results:
    unreal.log('[grab-rtg] RESULT %s' % r)
