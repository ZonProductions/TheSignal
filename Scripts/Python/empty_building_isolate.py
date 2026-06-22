import unreal
from collections import defaultdict, Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

ENV=("Backdrop_","Forest_Tree_","Ground_")
KEEP_MESH={
 "SM_SillingTile","SM_Woodfloor","SM_Ceiling_Main_E2","SM_Steps",
 "SM_WallBrace_A","SM_ElevatorWall","SM_Pipe",
 "SM_AirDuctF","SM_AirDuct_Vent","SM_AirDuctC","SM_AirDuctA","SM_AirDuctG",
 "SM_SpotLight_Hang","SM_HangedWire","Ultra_Dynamic_Sky_Sphere",
}
WALL_KEEP_LABEL=("windowwall","wallbrace")
CONTENT_CLASS={"ZP_InteractDoor","BP_Ladder_C","BP_Shambler_C","BP_Scytheer_C","ZP_ScytheerClimbPath"}

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
def has_glass(c):
    for i in range(c.get_num_materials()):
        m=c.get_material(i)
        if m and "glass" in m.get_name().lower(): return True
    return False

hid_m=Counter();hid_c=Counter();kep_m=Counter();kep_c=Counter()
for a in eas.get_all_level_actors():
    lbl=a.get_actor_label(); ll=lbl.lower(); cls=a.get_class().get_name()
    if lbl.startswith(ENV): a.set_is_temporarily_hidden_in_editor(False); kep_c.update([cls]); continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    c=cs[0] if cs else None
    sm=c.get_editor_property("static_mesh") if c else None
    if cls in CONTENT_CLASS:
        keep=False
    elif sm is not None:
        mn=sm.get_name(); mnl=mn.lower(); o,e=a.get_actor_bounds(False)
        if "door" in mnl: keep=False                       # doors + door frames always go
        elif mn in KEEP_MESH: keep=True
        elif mn=="SM_FrameTall": keep=True                 # window framing = shell
        elif mn=="SM_Cube":
            if e.z<80: keep=True                           # floor/ceiling slab
            elif has_glass(c): keep=True                   # glass window = shell
            elif any(k in ll for k in WALL_KEEP_LABEL): keep=True
            elif interior(o,e): keep=False                 # interior partition
            else: keep=True                                # exterior perimeter shell
        elif mnl in ("cylinder","sm_cylinder"): keep=(e.z>=150)
        else: keep=False                                   # named furniture/prop/fixture/item
    else:
        keep=True
    a.set_is_temporarily_hidden_in_editor(not keep)
    if keep:
        if sm: kep_m.update([sm.get_name()])
        else:  kep_c.update([cls])
    else:
        if sm: hid_m.update([sm.get_name()])
        else:  hid_c.update([cls])

print("HIDDEN total:", sum(hid_m.values())+sum(hid_c.values()))
print("KEPT total:", sum(kep_m.values())+sum(kep_c.values()))
print("\nRETURNED-to-shell now kept: SM_FrameTall=%d, glass SM_Cube windows kept"%kep_m.get("SM_FrameTall",0))
print("glass windows (SM_Cube) kept count check via no-longer-hidden; FrameTall hidden=%d"%hid_m.get("SM_FrameTall",0))
print("\nHIDDEN meshes (still-removed interior):")
for k,v in hid_m.most_common(): print("  %4d %s"%(v,k))
print("HIDDEN classes:")
for k,v in hid_c.most_common(): print("  %4d %s"%(v,k))
