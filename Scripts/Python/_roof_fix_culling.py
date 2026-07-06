import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
BACKUP = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_roof_backup_Z3380.json"
LABEL = "Roof_Consolidated_Z3380"

def add_hism(actor, name, relative_scale):
    handles = sds.k2_gather_subobject_data_for_instance(actor)
    root = handles[0]
    p = unreal.AddNewSubobjectParams()
    p.parent_handle = root
    p.new_class = unreal.HierarchicalInstancedStaticMeshComponent
    p.blueprint_context = None
    h, fail = sds.add_new_subobject(p)
    if not fail.is_empty():
        raise Exception("add_new_subobject failed: " + str(fail))
    sds.rename_subobject(h, unreal.Text.cast(name))
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h)
    comp = unreal.HierarchicalInstancedStaticMeshComponent.cast(
        unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data))
    comp.set_relative_scale3d(relative_scale)
    comp.set_static_mesh(unreal.load_asset(ROOF_MESH))
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    return comp

# --- load backup (exact original world transforms) ---
with open(BACKUP) as f:
    data = json.load(f)
tiles = data["tiles"]
print("Tiles in backup:", len(tiles))

pos_x, neg_x = [], []
sumx = sumy = 0.0
for t in tiles:
    sx, sy, sz = t["scale"]
    loc = unreal.Vector(*t["loc"])
    rot = unreal.Rotator(t["rot"][0], t["rot"][1], t["rot"][2])  # roll,pitch,yaw
    xf = unreal.Transform(loc, rot, unreal.Vector(sx, sy, sz))
    sumx += t["loc"][0]; sumy += t["loc"][1]
    det_sign = (1 if sx >= 0 else -1) * (1 if sy >= 0 else -1) * (1 if sz >= 0 else -1)
    (neg_x if det_sign < 0 else pos_x).append(xf)

print("Positive-det tiles:", len(pos_x), "| Negative-det (mirrored) tiles:", len(neg_x))
cx, cy = sumx/len(tiles), sumy/len(tiles)
print("Roof center approx: X=%.1f Y=%.1f" % (cx, cy))

# --- rebuild consolidated actor ---
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        print("Destroying old consolidated actor:", a.get_path_name())
        actor_sub.destroy_actor(a)

consolidated = actor_sub.spawn_actor_from_class(unreal.Actor, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
consolidated.set_actor_label(LABEL)

if pos_x:
    hp = add_hism(consolidated, "RoofHISM_Pos", unreal.Vector(1,1,1))
    hp.add_instances(pos_x, False, True)
    print("RoofHISM_Pos instances:", hp.get_instance_count())

hn = add_hism(consolidated, "RoofHISM_Neg", unreal.Vector(1,1,-1))  # mirrored component -> reversed culling
hn.add_instances(neg_x, False, True)
print("RoofHISM_Neg instances:", hn.get_instance_count())

# --- verify positions round-trip correctly through the mirrored component ---
worst = 0.0
for i in range(0, len(neg_x), max(1, len(neg_x)//20)):
    back = hn.get_instance_transform(i, True)  # world space
    want = neg_x[i].translation
    got = back.translation
    d = ((got.x-want.x)**2 + (got.y-want.y)**2 + (got.z-want.z)**2) ** 0.5
    worst = max(worst, d)
print("Worst position error on mirrored instances (sampled): %.5f uu" % worst)

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("SAVED. Actor:", consolidated.get_path_name())
