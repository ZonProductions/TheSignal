import unreal
from collections import Counter
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

mesh=Counter(); cls_nomesh=Counter()
env=0
for a in eas.get_all_level_actors():
    lbl=a.get_actor_label()
    if lbl.startswith(("Backdrop_","Forest_Tree_","Ground_")): env+=1; continue
    cs=a.get_components_by_class(unreal.StaticMeshComponent)
    sm=cs[0].get_editor_property("static_mesh") if cs else None
    if sm: mesh[sm.get_name()]+=1
    else:  cls_nomesh[a.get_class().get_name()]+=1

print("ENV actors (kept, skipped):", env)
print("\n=== ALL STATIC MESHES (name x count) — %d types ==="%len(mesh))
for k,v in mesh.most_common(): print("  %4d  %s"%(v,k))
print("\n=== NON-MESH ACTOR CLASSES (name x count) ==="    )
for k,v in cls_nomesh.most_common(): print("  %4d  %s"%(v,k))
