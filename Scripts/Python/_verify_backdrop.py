import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
zmins=[];zmaxs=[];xs=[];ys=[]
n=0
for a in eas.get_all_level_actors():
    if not a.get_actor_label().startswith("Backdrop_Building_"): continue
    org,ext=a.get_actor_bounds(False)
    zmins.append(org.z-ext.z); zmaxs.append(org.z+ext.z)
    xs.append(org.x); ys.append(org.y); n+=1
    if n<=4:
        print("  %s base_z=%.0f top_z=%.0f loc=(%.0f,%.0f)"%(a.get_actor_label(),org.z-ext.z,org.z+ext.z,org.x,org.y))
print("count=%d  base_z range[%.0f..%.0f]  top_z range[%.0f..%.0f]"%(n,min(zmins),max(zmins),min(zmaxs),max(zmaxs)))
print("X spread[%.0f..%.0f]  Y spread[%.0f..%.0f]"%(min(xs),max(xs),min(ys),max(ys)))
print("windows at z~900..1400 -> covered:", all(zt>1400 for zt in zmaxs) and all(zb<900 for zb in zmins))
