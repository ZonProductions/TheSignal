import unreal, json, time
out=[]
spec=json.load(open(r'C:/Users/Ommei/.claude/jobs/db244521/tmp/_crawler_spec.json'))
poses=json.load(open(r'C:/Users/Ommei/.claude/jobs/db244521/tmp/_crawler_poses.json'))
sk=unreal.load_asset('/Game/ProceduralMonster/ProceduralMonster/Meshes/Skeletal/SK_Tentacle')
at=unreal.AssetToolsHelpers.get_asset_tools(); ML=unreal.MathLibrary
def xf(loc,rot): return unreal.Transform(unreal.Vector(loc[0],loc[1],loc[2]), unreal.Rotator(roll=rot[0],pitch=rot[1],yaw=rot[2]), unreal.Vector(1,1,1))
actor='BP_TentacleLeg_C_5'
cw=None
for p in spec:
    if p['actor']==actor and p.get('mesh'): cw=xf(p['loc'],p['rot'])
pd=poses.get(actor+'/SkeletalMesh'); bones=pd['bones']
locals_=[]
for i,b in enumerate(bones):
    world=xf(b['l'],b['r']); parent=cw if i==0 else xf(bones[i-1]['l'],bones[i-1]['r'])
    locals_.append((b['b'], ML.make_relative_transform(world, parent)))
uid=int(time.time())%100000
name='A_CrawlerTPose5_%d'%uid
f=unreal.AnimSequenceFactory(); f.set_editor_property('target_skeleton', sk)
anim=at.create_asset(name,'/Game/Enemies/Crawler', unreal.AnimSequence, f)
out.append("create %s -> %s"%(name, anim.get_name() if anim else "None"))
if anim:
    ctrl=anim.get_editor_property('controller')
    ctrl.open_bracket("pose", False)
    ctrl.set_frame_rate(unreal.FrameRate(30,1), False)
    ctrl.set_number_of_frames(unreal.FrameNumber(1), False)
    for bn,lt in locals_:
        ctrl.add_bone_track(bn, False)
        ctrl.set_bone_track_keys(bn, [lt.translation,lt.translation], [lt.rotation,lt.rotation], [lt.scale3d,lt.scale3d], False)
    ctrl.close_bracket(False)
    unreal.EditorAssetLibrary.save_asset('/Game/Enemies/Crawler/'+name)
    out.append("authored tracks=%d len=%.3f"%(len(locals_), anim.get_play_length()))
    eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if a.get_actor_label()=='CrawlerPart_TentacleLeg_5':
            a.skeletal_mesh_component.play_animation(anim, False); out.append("applied to CrawlerPart_TentacleLeg_5")
open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_snap1_result.txt","w").write("\n".join(out))
