# Bakes the live (posed) PIE crawler to SM_Crawler_Ref via the C++ tool.
# Run this WHILE IN PIE, after a crawler has assembled (legs spread/curled).
import unreal
out=[]
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw=ues.get_game_world()
if gw is None:
    out.append("ERROR: not in PIE (game world is None). Press Play first.")
else:
    actors=unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.Actor)
    roots=[a for a in actors if a.get_class().get_name()=='BP_PatrolCreature_C']
    # pick the crawler with the most attached skeletal parts (most fully assembled)
    best=None; best_n=-1; best_set=None
    for r in roots:
        att=list(r.get_attached_actors(True, True))
        group=[r]+att
        nsk=0
        for x in group:
            for c in x.get_components_by_class(unreal.SkeletalMeshComponent):
                m=c.get_skeletal_mesh_asset() if hasattr(c,'get_skeletal_mesh_asset') else None
                if m: nsk+=1
        if nsk>best_n: best_n=nsk; best=r; best_set=group
    out.append("crawlers=%d; best=%s attached_group=%d skelComps=%d" % (len(roots), best.get_actor_label() if best else None, len(best_set) if best_set else 0, best_n))
    if best and best_n>0:
        mesh=unreal.ZP_MeshBakeUtils.bake_actors_to_static_mesh(best_set, "/Game/Enemies/Crawler/SM_Crawler_Ref", False)
        out.append("BAKE RESULT: %s" % (mesh.get_path_name() if mesh else "None (see Output Log [MeshBake])"))
open(r"C:/Users/Ommei/.claude/jobs/db244521/tmp/_fab_result.txt","w").write("\n".join(out))
print("\n".join(out))
