import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
txs=[];tys=[];hts=[];near=10**9
INNER=(-19000,11500,-12000,16000)
inside=0
for a in eas.get_all_level_actors():
    if not a.get_actor_label().startswith("Forest_Tree_"): continue
    l=a.get_actor_location(); org,ext=a.get_actor_bounds(False)
    txs.append(l.x); tys.append(l.y); hts.append(ext.z*2)
    if INNER[0]<=l.x<=INNER[1] and INNER[2]<=l.y<=INNER[3]: inside+=1
    # nearest tree distance to building center (3755,2423)
    d=((l.x-3755)**2+(l.y-2423)**2)**0.5; near=min(near,d)
print("trees:",len(txs))
print("X[%.0f..%.0f] Y[%.0f..%.0f]"%(min(txs),max(txs),min(tys),max(tys)))
print("tree height range: %.0f..%.0f UU (%.0f..%.0f m)"%(min(hts),max(hts),min(hts)/100,max(hts)/100))
print("trees inside backdrop-building ring (should be 0):",inside)
print("nearest tree to building center: %.0f UU (%.0f m)"%(near,near/100))
# backdrop building extent for reference
bxs=[];bys=[]
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Backdrop_Building_"):
        l=a.get_actor_location(); bxs.append(l.x); bys.append(l.y)
print("backdrop buildings X[%.0f..%.0f] Y[%.0f..%.0f]"%(min(bxs),max(bxs),min(bys),max(bys)))
