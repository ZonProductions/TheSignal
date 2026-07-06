import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)

ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
TARGET_Z = 3380.434814
TOL = 5.0
BACKUP = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_roof_backup_Z3380.json"
LABEL = "Roof_Consolidated_Z3380"

actors = actor_sub.get_all_level_actors()

# --- idempotency: remove any prior consolidated actor (e.g. orphan from a failed run) ---
for a in actors:
    if a.get_actor_label() == LABEL:
        print("Removing prior actor:", a.get_path_name())
        actor_sub.destroy_actor(a)
actors = actor_sub.get_all_level_actors()

# --- collect the roof tiles ---
tiles = []
for a in actors:
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH and abs(a.get_actor_location().z - TARGET_Z) < TOL:
        tiles.append((a, smc))

print("Roof tiles to consolidate:", len(tiles))
if not tiles:
    raise Exception("No tiles found - aborting.")

# --- backup originals to JSON (reversibility) ---
backup = []
xforms = []
for a, smc in tiles:
    wt = smc.get_world_transform()
    xforms.append(wt)
    loc = wt.translation; rot = wt.rotation.rotator(); scl = wt.scale3d
    mats = smc.get_editor_property('override_materials')
    backup.append({
        "path": a.get_path_name(),
        "label": a.get_actor_label(),
        "loc": [loc.x, loc.y, loc.z],
        "rot": [rot.roll, rot.pitch, rot.yaw],
        "scale": [scl.x, scl.y, scl.z],
        "overrides": [m.get_path_name() if m else None for m in mats],
    })
with open(BACKUP, "w") as f:
    json.dump({"mesh": ROOF_MESH, "target_z": TARGET_Z, "count": len(backup), "tiles": backup}, f, indent=1)
print("Backup written:", BACKUP)

# --- spawn consolidated actor at origin ---
mesh = unreal.load_asset(ROOF_MESH)
consolidated = actor_sub.spawn_actor_from_class(unreal.Actor, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
consolidated.set_actor_label(LABEL)

# --- add a HISM as an instance component via SubobjectDataSubsystem ---
handles = sds.k2_gather_subobject_data_for_instance(consolidated)
root_handle = handles[0]
params = unreal.AddNewSubobjectParams()
params.parent_handle = root_handle
params.new_class = unreal.HierarchicalInstancedStaticMeshComponent
params.blueprint_context = None
new_handle, fail = sds.add_new_subobject(params)
if not fail.is_empty():
    raise Exception("add_new_subobject failed: " + str(fail))
sds.rename_subobject(new_handle, unreal.Text.cast("RoofHISM"))
data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(new_handle)
hism = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)
hism = unreal.HierarchicalInstancedStaticMeshComponent.cast(hism)

hism.set_static_mesh(mesh)
hism.set_mobility(unreal.ComponentMobility.STATIC)

# add all instances in WORLD space (actor at origin => local == world)
hism.add_instances(xforms, False, True)
print("Instances added:", hism.get_instance_count())
print("Consolidated actor path:", consolidated.get_path_name())

# save level
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("SAVED. Final instance count:", hism.get_instance_count())
