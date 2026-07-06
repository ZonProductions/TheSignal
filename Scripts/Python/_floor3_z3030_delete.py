import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
TARGET_Z = 3030.0
TOL = 5.0
BACKUP = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_floor3_Z3030_backup.json"

# --- PIE guard ---
if ues.get_editor_world() is None:
    raise Exception("ABORT: editor world is None (in PIE). Stop PIE before deleting.")

actors = actor_sub.get_all_level_actors()
print("Total level actors:", len(actors))

# collect SM_Floor_3 StaticMeshActors at Z=3030
victims = []
for a in actors:
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH and abs(a.get_actor_location().z - TARGET_Z) < TOL:
        victims.append((a, smc))

print("SM_Floor_3 @ Z=3030 matched:", len(victims))
if not victims:
    raise Exception("ABORT: nothing matched.")

# backup (reversibility)
backup = []
for a, smc in victims:
    wt = smc.get_world_transform()
    loc = wt.translation; rot = wt.rotation.rotator(); scl = wt.scale3d
    mats = smc.get_editor_property('override_materials')
    backup.append({
        "path": a.get_path_name(), "label": a.get_actor_label(),
        "loc": [loc.x, loc.y, loc.z], "rot": [rot.roll, rot.pitch, rot.yaw],
        "scale": [scl.x, scl.y, scl.z],
        "overrides": [m.get_path_name() if m else None for m in mats],
    })
with open(BACKUP, "w") as f:
    json.dump({"mesh": ROOF_MESH, "target_z": TARGET_Z, "count": len(backup), "tiles": backup}, f, indent=1)
print("Backup written:", BACKUP)

# delete
deleted = 0
for a, smc in victims:
    if actor_sub.destroy_actor(a):
        deleted += 1
print("Deleted:", deleted)

# save + verify
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
remaining = 0
for a in actor_sub.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH and abs(a.get_actor_location().z - TARGET_Z) < TOL:
        remaining += 1
print("Remaining SM_Floor_3 @ Z=3030:", remaining)
print("DONE")
