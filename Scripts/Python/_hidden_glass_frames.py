import unreal
from collections import defaultdict, Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ENV=("Backdrop_","Forest_Tree_","Ground_")

# rebuild floor rects + interior test
slabs=[]; areaz=defaultdict(float)
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(ENV): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs or not cs[0].get_editor_property("static_mesh"): continue
    o,e=a.get_actor_bounds(False)
    if e.z<80 and 150<e.x<10000 and 150<e.y<10000 and 850<o.z<1100:
        slabs.append((o,e)); areaz[int(o.z//25)*25]+=e.x*e.y
fz=max(areaz,key=areaz.get) if areaz else 986
rects=[(o.x-e.x,o.y-e.y,o.x+e.x,o.y+e.y) for o,e in slabs if abs(o.z-fz)<70]
def overf(x,y):
    for a0,b0,a1,b1 in rects:
        if a0<=x<=a1 and b0<=y<=b1: return True
    return False
def interior(o,e):
    tx=e.x<=e.y; le=e.y if tx else e.x; s1=s2=0
    for t in(-0.4,0,0.4):
        if tx: cx,cy=o.x,o.y+t*le; p1=(cx-(e.x+200),cy);p2=(cx+(e.x+200),cy)
        else:  cx,cy=o.x+t*le,o.y; p1=(cx,cy-(e.y+200));p2=(cx,cy+(e.y+200))
        s1+=overf(*p1); s2+=overf(*p2)
    return s1>=2 and s2>=2

# currently-hidden actors that are framing or glass-bearing -> classify
ext=Counter(); inte=Counter()
samples=defaultdict(list)
for a in eas.get_all_level_actors():
    if not a.is_temporarily_hidden_in_editor(): continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    c=cs[0]; sm=c.get_editor_property("static_mesh")
    if not sm: continue
    mn=sm.get_name(); mats=[c.get_material(i).get_name() for i in range(c.get_num_materials()) if c.get_material(i)]
    glass = any("glass" in m.lower() for m in mats)
    if not (mn=="SM_FrameTall" or mn=="Plane" or glass): continue
    o,e=a.get_actor_bounds(False)
    verdict = "INTERIOR" if interior(o,e) else "EXTERIOR"
    key=(mn, verdict, "glass" if glass else "noglass")
    (ext if verdict=="EXTERIOR" else inte)[mn]+=1
    if len(samples[key])<3:
        samples[key].append("%s loc=(%.0f,%.0f,%.0f) mats=%s"%(a.get_actor_label(),o.x,o.y,o.z,mats))
print("EXTERIOR (should RETURN):", dict(ext))
print("INTERIOR (stay removed):", dict(inte))
print("\nsamples:")
for k,v in samples.items():
    print(" ",k)
    for s in v: print("     ",s)
