"""
Retarget the CharacterCustomizer SLS turn-in-place clips (dev pick, 2026-07-14) onto the
Shambler's necromorph skeleton for the new alert-turn phase:

  /Game/CharacterCustomizer/Components/SLS/Animations/Idle/Turn_Left_90   -> A_Shambler_Turn_L90
  .../Turn_Right_90  -> A_Shambler_Turn_R90
  .../Turn_Left_180  -> A_Shambler_Turn_L180
  .../Turn_Right_180 -> A_Shambler_Turn_R180

Pipeline = the proven grab-batch pattern (Scripts/Python/retarget_grab_anims.py):
  - source rig IK_CCMH_Split (split Neck/Head chains — the combined-chain CC rig is the
    2026-07-02 neck-doubling DEAD END), source mesh CCMH_Body_Male (same source used for the
    SLS Get_Up_Back retarget, proven compatible with SLS clips);
  - target rig IK_Shambler + necromorph mesh (the rigs behind every existing shambler clip);
  - RTG created idempotently with add_default_ops + run_op_initial_setup(i) (UE5.8 empty-op-
    stack gotcha), chain mapping printed for verification;
  - every output CURVE-STRIPPED (DEAD ENDS 2026-06-29 PostLoad crash) + motion-verified.
Idempotent: existing outputs are skipped (never Python-overwrite an asset).
"""
import unreal

AR = unreal.AssetRegistryHelpers.get_asset_registry()

CCMH_MESH = '/Game/CharacterCustomizer/Characters/CCMH/Meshes/CCMH_Body_Male'
NECRO_MESH = '/Game/Enemies/Shambler/SkeletalMeshes/zombie_monster_slasher_necromorph'
IK_SPLIT = '/Game/Marcus/Rigs/IK_CCMH_Split'
IK_SHAM = '/Game/Enemies/Shambler/Rigs/IK_Shambler'
RTG_PATH = '/Game/Enemies/Shambler/Rigs'
RTG_NAME = 'RTG_CC_to_Shambler'
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
    cls = str(ad.asset_class_path.asset_name) if ad else 'None'
    if cls != 'AnimSequence':
        raise RuntimeError('AssetData class for %s is %s (want AnimSequence)' % (path, cls))
    return ad


# --- sanity: source clips' skeleton must be the CCMH/CC family the split rig is bound to ----
src_skel = load('%s/Turn_Left_90' % SRC_DIR).get_editor_property('skeleton').get_path_name()
ccmh_skel = load(CCMH_MESH).get_editor_property('skeleton').get_path_name()
print('SLS clip skeleton:  %s' % src_skel)
print('CCMH mesh skeleton: %s' % ccmh_skel)
if src_skel != ccmh_skel:
    print('WARNING: skeletons differ — retarget may still work if same family, verify output!')

# --- retargeter (idempotent) ---------------------------------------------------------------
full_rtg = '%s/%s' % (RTG_PATH, RTG_NAME)
if unreal.EditorAssetLibrary.does_asset_exist(full_rtg):
    rtg = load(full_rtg)
    print('REUSING %s' % full_rtg)
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rtg = tools.create_asset(RTG_NAME, RTG_PATH, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, load(IK_SPLIT))
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, load(IK_SHAM))
    try:
        rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, load(CCMH_MESH))
        rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, load(NECRO_MESH))
    except Exception as e:
        print('set_preview_mesh: %s (non-fatal)' % e)
    if rc.get_num_retarget_ops() == 0:
        rc.add_default_ops()
    for i in range(rc.get_num_retarget_ops()):
        rc.run_op_initial_setup(i)
    unreal.EditorAssetLibrary.save_asset(full_rtg)
    print('CREATED %s (%d ops)' % (full_rtg, rc.get_num_retarget_ops()))

# Print the chain mapping so neck/spine sanity is verifiable in the log.
try:
    rc = unreal.IKRetargeterController.get_controller(rtg)
    for m in (rc.get_chain_mappings() or []):
        print('CHAIN MAP: %s -> %s' % (m.get_editor_property('source_chain'), m.get_editor_property('target_chain')))
except Exception as e:
    print('chain-map dump unavailable: %s' % e)

# --- retarget + strip + verify ---------------------------------------------------------------
ccmh = load(CCMH_MESH)
necro = load(NECRO_MESH)
for src_name, out_name in CLIPS:
    src_path = '%s/%s' % (SRC_DIR, src_name)
    out_full = '%s/%s' % (OUT_DIR, out_name)
    if unreal.EditorAssetLibrary.does_asset_exist(out_full):
        print('SKIP (exists): %s' % out_full)
        continue
    new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [asset_data(src_path)], ccmh, necro, rtg,
        search=src_name, replace=out_name,
        target_path=OUT_DIR, overwrite_existing_files=False)
    if not new or len(new) == 0:
        raise RuntimeError('duplicate_and_retarget produced NOTHING for %s' % src_path)
    if not unreal.EditorAssetLibrary.does_asset_exist(out_full):
        for a in unreal.EditorAssetLibrary.list_assets(OUT_DIR, recursive=False):
            base = a.split('/')[-1].split('.')[0]
            if src_name in base:
                unreal.EditorAssetLibrary.rename_asset(a.split('.')[0], out_full)
                break
    # strip curves (PostLoad crash lesson) + verify motion
    anim = load(out_full)
    skel_path = anim.get_editor_property('skeleton').get_path_name()
    names = [str(c) for c in (unreal.AnimationLibrary.get_animation_curve_names(
        anim, unreal.RawCurveTrackTypes.RCT_FLOAT) or [])]
    for n in names:
        unreal.AnimationLibrary.remove_curve(anim, n, False)
    length = unreal.AnimationLibrary.get_sequence_length(anim)
    moving = False
    probe_used = None
    for b in ('pelvis', 'spine_01', 'thigh_l', 'Hips', 'Spine', 'LeftUpLeg'):
        try:
            e0 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, 0.0, False).rotation.euler()
            e1 = unreal.AnimationLibrary.get_bone_pose_for_time(anim, b, length * 0.5, False).rotation.euler()
            d = (e1 - e0).length()
            probe_used = b
            if d > 1.0:
                moving = True
                break
        except Exception:
            continue
    saved = unreal.EditorAssetLibrary.save_asset(out_full)
    print('OUT %s len=%.3f stripped=%d probe=%s moving=%s saved=%s skel=%s' % (
        out_full, length, len(names), probe_used, moving, saved, skel_path.split('/')[-1]))
    if not moving:
        print('ERROR: STATIC OUTPUT (empty op stack / bad chain map?): %s' % out_full)
