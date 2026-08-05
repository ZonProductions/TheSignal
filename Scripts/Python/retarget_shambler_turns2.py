"""
Shambler turn-clip retarget, ATTEMPT 2 (2026-07-14). Attempt 1 produced STATIC outputs:
the SLS Idle turn clips live on CC_Skeleton (CC_Dummy), not CCMH — and CC_IK_Rig's chain
names are bone-style, so nothing auto-maps to IK_Shambler's chains (Spine/Neck/Left*/Right*).

This pass:
  1. deletes attempt 1's OWN broken artifacts (4 static clips + the misconfigured RTG) —
     created minutes ago by this pipeline, not dev content;
  2. authors IK_CCDummy_Shambler on CC_Dummy with EXACTLY IK_Shambler's 8 chain names
     (auto-map pairs by name; target has combined Neck + no Head chain, so no neck-doubling
     topology risk — source Neck spans neck_01->head to match a multi-bone target Neck);
  3. creates RTG_CC_to_Shambler (source IK_CCDummy_Shambler/CC_Dummy, target IK_Shambler/
     necromorph) with add_default_ops + run_op_initial_setup;
  4. retargets the 4 turn clips, strips curves, verifies MOTION.
"""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

CC_DUMMY = '/Game/CharacterCustomizer/CharacterCustomizer_Core/Pawns/CC_Dummy/CC_Dummy'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
IK_SHAM = '/Game/Enemies/Shambler/Rigs/IK_Shambler'
RIGS_DIR = '/Game/Enemies/Shambler/Rigs'
OUT_DIR = '/Game/Enemies/Shambler/Anims'
SRC_DIR = '/Game/CharacterCustomizer/Components/SLS/Animations/Idle'

CLIPS = [
    ('Turn_Left_90', 'A_Shambler_Turn_L90'),
    ('Turn_Right_90', 'A_Shambler_Turn_R90'),
    ('Turn_Left_180', 'A_Shambler_Turn_L180'),
    ('Turn_Right_180', 'A_Shambler_Turn_R180'),
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


# 1. Clean up attempt 1's broken artifacts (mine, minutes old, all static/misconfigured)
for junk in ['%s/%s' % (OUT_DIR, out) for _, out in CLIPS] + ['%s/RTG_CC_to_Shambler' % RIGS_DIR]:
    if unreal.EditorAssetLibrary.does_asset_exist(junk):
        ok = unreal.EditorAssetLibrary.delete_asset(junk)
        print('DELETE broken attempt-1 artifact %s: %s' % (junk, ok))

# 2. Source rig on CC_Dummy, chain names mirroring IK_Shambler exactly
IK_SRC = '%s/IK_CCDummy_Shambler' % RIGS_DIR
if not unreal.EditorAssetLibrary.does_asset_exist(IK_SRC):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rig = tools.create_asset('IK_CCDummy_Shambler', RIGS_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    rc = unreal.IKRigController.get_controller(rig)
    rc.set_skeletal_mesh(load(CC_DUMMY))
    rc.set_retarget_root('pelvis')
    CHAINS = [
        ('Spine', 'spine_01', 'spine_03'),
        ('Neck', 'neck_01', 'head'),
        ('LeftClavicle', 'clavicle_l', 'clavicle_l'),
        ('RightClavicle', 'clavicle_r', 'clavicle_r'),
        ('LeftArm', 'upperarm_l', 'hand_l'),
        ('RightArm', 'upperarm_r', 'hand_r'),
        ('LeftLeg', 'thigh_l', 'foot_l'),
        ('RightLeg', 'thigh_r', 'foot_r'),
    ]
    ok = 0
    for name, sb, eb in CHAINS:
        try:
            rc.add_retarget_chain(name, sb, eb, 'None')
            ok += 1
        except Exception as e:
            print('chain %s failed: %s' % (name, e))
    unreal.EditorAssetLibrary.save_asset(IK_SRC)
    print('CREATED %s (%d/8 chains)' % (IK_SRC, ok))
else:
    print('REUSING %s' % IK_SRC)

# 3. Retargeter
RTG = '%s/RTG_CC_to_Shambler' % RIGS_DIR
tools = unreal.AssetToolsHelpers.get_asset_tools()
rtg = tools.create_asset('RTG_CC_to_Shambler', RIGS_DIR, unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, load(IK_SRC))
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, load(IK_SHAM))
try:
    rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, load(CC_DUMMY))
    rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, load(NECRO_MESH))
except Exception as e:
    print('set_preview_mesh: %s (non-fatal)' % e)
if rc.get_num_retarget_ops() == 0:
    rc.add_default_ops()
for i in range(rc.get_num_retarget_ops()):
    rc.run_op_initial_setup(i)
unreal.EditorAssetLibrary.save_asset(RTG)
print('CREATED %s (%d ops)' % (RTG, rc.get_num_retarget_ops()))

# 4. Retarget + strip + verify motion
cc = load(CC_DUMMY)
necro = load(NECRO_MESH)
for src_name, out_name in CLIPS:
    src_path = '%s/%s' % (SRC_DIR, src_name)
    out_full = '%s/%s' % (OUT_DIR, out_name)
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(src_path)], cc, necro, rtg,
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
    for n in [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
            anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]:
        unreal.AnimationLibrary.remove_curve(anim, n, False)
    # 2026-08-04 lesson: the SLS Idle/Turn_* SOURCES are ADDITIVE (AAT_LOCAL_SPACE_BASE vs
    # frame 0) with force_root_lock=True, and duplicate_and_retarget COPIES those flags onto
    # the output. On the necromorph (hips IS the root; ref-pose hips scale 0.0001) either flag
    # collapses the evaluated pose -> INVISIBLE mesh + blank previews. Clear them so the
    # retargeted full-pose tracks play as absolute animation.
    anim.set_editor_property('additive_anim_type', unreal.AdditiveAnimationType.AAT_NONE)
    anim.set_editor_property('ref_pose_type', unreal.AdditiveBasePoseType.ABPT_NONE)
    anim.set_editor_property('force_root_lock', False)
    length = unreal.AnimationLibrary.get_sequence_length(anim)
    moving = False
    probe = None
    for b in ('mixamorig_Hips_01', 'mixamorig_Spine_02', 'mixamorig_LeftUpLeg_03', 'mixamorig_RightLeg_04'):
        try:
            e0 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, 0.0, False).rotation.euler()
            e1 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, length * 0.5, False).rotation.euler()
            probe = b
            if (e1 - e0).length() > 1.0:
                moving = True
                break
        except Exception:
            continue
    saved = unreal.EditorAssetLibrary.save_asset(out_full)
    print('OUT %s len=%.3f probe=%s MOVING=%s saved=%s' % (out_full, length, probe, moving, saved))
    if not moving:
        print('ERROR: STILL STATIC: %s' % out_full)
