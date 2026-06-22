import unreal
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = ues.get_editor_world()
print("CURRENT MAP:", w.get_name() if w else None)

# wall-type mesh tally + Z spread
from collections import Counter
wall_meshes = Counter()
wallbrick = 0
allcount = 0
zspread=[]
for a in eas.get_all_level_actors():
    allcount+=1
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    if not cs: continue
    sm = cs[0].get_editor_property("static_mesh")
    if not sm: continue
    mn = sm.get_name()
    lbl = a.get_actor_label()
    if "wall" in mn.lower() or "wall" in lbl.lower() or "brick" in lbl.lower():
        wall_meshes[mn]+=1
        zspread.append(a.get_actor_location().z)
print("total actors:", allcount)
print("wall-ish mesh tally:", dict(wall_meshes))
if zspread:
    zs=sorted(zspread); print("wall Z: min=%.0f max=%.0f med=%.0f count=%d"%(zs[0],zs[-1],zs[len(zs)//2],len(zs)))
