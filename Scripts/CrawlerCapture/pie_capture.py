import unreal, json
out=[]
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world=ues.get_game_world()
out.append("game world=%s" % (world.get_name() if world else "NONE"))
crawlers=[]
try: crawlers=list(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ZP_CrawlerBase))
except Exception as e: out.append("err: %s" % e)
if not crawlers and world:
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        n=a.get_name().lower()
        if 'patrolcreature' in n or 'monster' in n: crawlers.append(a)
out.append("crawlers: %d -> %s" % (len(crawlers), [c.get_name() for c in crawlers][:6]))
spec=[]
if crawlers:
    cr=crawlers[0]
    actors=[cr]
    try: actors += list(cr.get_attached_actors(True, True))
    except Exception as e: out.append("attached err: %s" % e)
    for a in list(actors):
        for cac in a.get_components_by_class(unreal.ChildActorComponent):
            try:
                ca=cac.get_editor_property('child_actor')
                if ca and ca not in actors: actors.append(ca)
            except: pass
    out.append("crawler='%s' actors=%d: %s" % (cr.get_name(), len(actors), [a.get_name() for a in actors][:24]))
    for a in actors:
        for smc in a.get_components_by_class(unreal.SkeletalMeshComponent):
            try:
                sm=smc.get_skeletal_mesh_asset()
                wt=smc.get_world_transform()
                spec.append({'actor':a.get_name(),'comp':smc.get_name(),'kind':'skel','mesh':sm.get_path_name() if sm else None,
                    'loc':[round(wt.translation.x,1),round(wt.translation.y,1),round(wt.translation.z,1)],
                    'rot':[round(wt.rotation.euler().x,1),round(wt.rotation.euler().y,1),round(wt.rotation.euler().z,1)],
                    'scale':[round(wt.scale3d.x,3),round(wt.scale3d.y,3),round(wt.scale3d.z,3)],'bones':smc.get_num_bones()})
            except Exception as e: pass
        for stc in a.get_components_by_class(unreal.StaticMeshComponent):
            try:
                sm=stc.get_static_mesh()
                if not sm: continue
                wt=stc.get_world_transform()
                spec.append({'actor':a.get_name(),'comp':stc.get_name(),'kind':'static','mesh':sm.get_path_name(),
                    'loc':[round(wt.translation.x,1),round(wt.translation.y,1),round(wt.translation.z,1)],
                    'rot':[round(wt.rotation.euler().x,1),round(wt.rotation.euler().y,1),round(wt.rotation.euler().z,1)],
                    'scale':[round(wt.scale3d.x,3),round(wt.scale3d.y,3),round(wt.scale3d.z,3)]})
            except Exception as e: pass
    from collections import Counter
    out.append("captured %d parts; by mesh: %s" % (len(spec), dict(Counter([(s['mesh'].split('.')[-1] if s.get('mesh') else None) for s in spec]))))
json.dump(spec, open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_crawler_spec.json","w"), indent=1)
open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_fab_result.txt","w").write("\n".join(out))
