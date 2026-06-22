import unreal, re
from collections import Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# building footprint from solid structure (exclude env + degenerate)
xs=[];ys=[]
walls=[]
for a in eas.get_all_level_actors():
    l=a.get_actor_label()
    if l.startswith(("Backdrop_","Forest_Tree_","Ground_")): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm=cs[0].get_editor_property("static_mesh")
    if not sm: continue
    org,ext=a.get_actor_bounds(False)
    if ext.x>40000 or ext.y>40000: continue
    if abs(org.x)>30000 or abs(org.y)>30000: continue
    xs+=[org.x-ext.x,org.x+ext.x]; ys+=[org.y-ext.y,org.y+ext.y]
    mn=sm.get_name()
    if "wall" in mn.lower() or "wall" in l.lower() or "brick" in l.lower():
        walls.append((a,l,mn,org,ext))
X0,X1,Y0,Y1=min(xs),max(xs),min(ys),max(ys)
print("building footprint X[%.0f..%.0f] Y[%.0f..%.0f]"%(X0,X1,Y0,Y1))

# label-prefix tally of walls
pref=Counter()
for a,l,mn,org,ext in walls:
    m=re.match(r'^([A-Za-z_]+?)(\d|$)', l)
    pref[m.group(1) if m else l]+=1
print("\nwall label-prefix tally:")
for k,v in pref.most_common(): print("  %4d  %s"%(v,k))

# perimeter test: wall touching footprint edge (within 600 UU) = exterior
EDGE=600.0
ext_cnt=0; int_cnt=0
ext_ex={}; int_ex={}
for a,l,mn,org,ext in walls:
    near_edge = (abs((org.x-ext.x)-X0)<EDGE or abs((org.x+ext.x)-X1)<EDGE or
                 abs((org.y-ext.y)-Y0)<EDGE or abs((org.y+ext.y)-Y1)<EDGE)
    key=re.match(r'^([A-Za-z_]+)', l).group(1)
    if near_edge: ext_cnt+=1; ext_ex[key]=ext_ex.get(key,0)+1
    else: int_cnt+=1; int_ex[key]=int_ex.get(key,0)+1
print("\nperimeter(EXTERIOR, keep)=%d  interior(remove)=%d"%(ext_cnt,int_cnt))
print("exterior label tally:", ext_ex)
print("interior label tally:", int_ex)
