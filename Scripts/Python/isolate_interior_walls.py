import unreal
from collections import defaultdict, Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

ENVPREFIX = ("Backdrop_","Forest_Tree_","Ground_")
ALWAYS_KEEP = ("windowwall","wallbrace")   # exterior shell + structural brace

# floor level + rects
slabs=[]; areaz=defaultdict(float)
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(ENVPREFIX): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs or not cs[0].get_editor_property("static_mesh"): continue
    org,ext=a.get_actor_bounds(False)
    if ext.z<80 and 150<ext.x<10000 and 150<ext.y<10000 and 850<org.z<1100:
        slabs.append((org,ext)); areaz[int(org.z//25)*25]+=ext.x*ext.y
floor_z=max(areaz,key=areaz.get) if areaz else 986
rects=[(o.x-e.x,o.y-e.y,o.x+e.x,o.y+e.y) for (o,e) in slabs if abs(o.z-floor_z)<70]
print("floor_z~%d rects=%d"%(floor_z,len(rects)))
def over_floor(x,y):
    for rx0,ry0,rx1,ry1 in rects:
        if rx0<=x<=rx1 and ry0<=y<=ry1: return True
    return False

P=200.0
def is_interior(org,ext):
    thinX=ext.x<=ext.y; le=ext.y if thinX else ext.x; s1=s2=0
    for t in (-0.4,0.0,0.4):
        if thinX: cx,cy=org.x,org.y+t*le; p1=(cx-(ext.x+P),cy);p2=(cx+(ext.x+P),cy)
        else:     cx,cy=org.x+t*le,org.y; p1=(cx,cy-(ext.y+P));p2=(cx,cy+(ext.y+P))
        s1+=over_floor(*p1); s2+=over_floor(*p2)
    return s1>=2 and s2>=2

hid=Counter(); keptext=Counter(); shell=Counter()
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(ENVPREFIX): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm=cs[0].get_editor_property("static_mesh")
    if not sm: continue
    mn=sm.get_name(); mnl=mn.lower(); lbl=a.get_actor_label().lower()
    org,ext=a.get_actor_bounds(False)
    minh,maxh=min(ext.x,ext.y),max(ext.x,ext.y)
    name_wall  = ("wall" in mnl or "wall" in lbl or "brick" in lbl)
    shape_wall = (ext.z>=120 and minh<=120 and maxh>=150)   # any-mesh vertical panel
    if not (name_wall or shape_wall):
        a.set_is_temporarily_hidden_in_editor(False); continue
    if any(k in lbl or k in mnl for k in ALWAYS_KEEP):
        shell[mn]+=1; a.set_is_temporarily_hidden_in_editor(False); continue
    if is_interior(org,ext):
        hid[mn]+=1; a.set_is_temporarily_hidden_in_editor(True)
    else:
        keptext[mn]+=1; a.set_is_temporarily_hidden_in_editor(False)

print("\nHIDDEN interior walls (to delete): %d"%sum(hid.values()))
for k,v in hid.most_common(): print("   %4d  %s"%(v,k))
print("KEPT exterior walls (shell): %d"%sum(keptext.values()))
for k,v in keptext.most_common(): print("   %4d  %s"%(v,k))
print("ALWAYS-KEPT WindowWall/WallBrace: %d"%sum(shell.values()))
