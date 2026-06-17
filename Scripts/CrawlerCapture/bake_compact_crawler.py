# Run IN PIE. Picks the MOST COMPACT crawler (legs gathered) and bakes it.
import unreal
out=[]
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw=ues.get_game_world()
if gw is None:
    out.append("ERROR: not in PIE.")
else:
    roots=[a for a in unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor) if a.get_class().get_name()=='BP_PatrolCreature_C']
    def crawler_bounds(r):
        mn=[1e9,1e9,1e9]; mx=[-1e9,-1e9,-1e9]
        for a in [r]+list(r.get_attached_actors(True,True)):
            o,e=a.get_actor_bounds(False)
            for i,(oo,ee) in enumerate(((o.x,e.x),(o.y,e.y),(o.z,e.z))):
                if ee<=0: continue
                mn[i]=min(mn[i],oo-ee); mx[i]=max(mx[i],oo+ee)
        return [mx[i]-mn[i] for i in range(3)]
    scored=[]
    for r in roots:
        d=crawler_bounds(r); scored.append((max(d), r, d))
        out.append("%s dims=(%.0f,%.0f,%.0f) maxdim=%.0f"%(r.get_actor_label(),d[0],d[1],d[2],max(d)))
    scored.sort(key=lambda t:t[0])
    md,best,d=scored[0]
    out.append(">>> baking MOST COMPACT: %s maxdim=%.0f"%(best.get_actor_label(),md))
    group=[best]+list(best.get_attached_actors(True,True))
    mesh=unreal.ZP_MeshBakeUtils.bake_actors_to_static_mesh(group, "/Game/Enemies/Crawler/SM_CrawlerRefV2", False)
    if mesh:
        b=mesh.get_bounds(); e=b.box_extent
        out.append("BAKED %s tris=%d size=(%.0f,%.0f,%.0f)"%(mesh.get_path_name(), mesh.get_num_triangles(0), e.x*2,e.y*2,e.z*2))
    else:
        out.append("BAKE FAILED (see [MeshBake] log)")
open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_fab_result.txt","w").write("\n".join(out))
