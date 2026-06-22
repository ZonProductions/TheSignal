import unreal
from collections import Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sub=Counter()
samples={}
for a in eas.get_all_level_actors():
    l=a.get_actor_label()
    if l.startswith(("Backdrop_","Forest_Tree_","Ground_")): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm=cs[0].get_editor_property("static_mesh")
    if not sm: continue
    mn=sm.get_name()
    if not ("wall" in mn.lower() or "wall" in l.lower() or "brick" in l.lower()): continue
    # bucket by meaningful token
    ll=l.lower()
    if "windowwall" in ll: key="WindowWall(exterior-converted)"
    elif "wallbrick" in ll: key="wallBrick(interior partition)"
    elif "wcwall" in ll: key="WcWall(bathroom)"
    elif "elevatorwall" in ll: key="ElevatorWall"
    elif "wallbrace" in ll: key="WallBrace"
    else: key="OTHER:%s/%s"%(l,mn)
    sub[key]+=1
    samples.setdefault(key,[]).append("%s[%s]"%(l,mn))
for k,v in sub.most_common():
    print("%4d  %s"%(v,k))
    print("      e.g.", ", ".join(samples[k][:3]))
