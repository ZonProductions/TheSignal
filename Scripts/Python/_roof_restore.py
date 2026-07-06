import unreal, json

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
BACKUP = "C:/Users/Ommei/workspace/TheSignal/Scripts/Python/_roof_backup_Z3380.json"
LABEL = "Roof_Consolidated_Z3380"

# PIE guard
if ues.get_editor_world() is None:
    raise Exception("ABORT: editor world is None (in PIE). Stop PIE first.")

with open(BACKUP) as f:
    data = json.load(f)
tiles = data["tiles"]
print("Tiles to restore:", len(tiles))

mesh = unreal.load_asset(ROOF_MESH)
mat_cache = {}

restored = 0
for t in tiles:
    loc = unreal.Vector(t["loc"][0], t["loc"][1], t["loc"][2])
    rot = unreal.Rotator(t["rot"][0], t["rot"][1], t["rot"][2])  # roll,pitch,yaw
    a = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    smc.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(t["scale"][0], t["scale"][1], t["scale"][2]))
    for i, m in enumerate(t.get("overrides", [])):
        if m:
            if m not in mat_cache:
                mat_cache[m] = unreal.load_asset(m)
            smc.set_material(i, mat_cache[m])
    a.set_actor_label(t["label"])
    restored += 1

print("Restored StaticMeshActors:", restored)

# remove the consolidated actor(s)
removed = 0
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        actor_sub.destroy_actor(a); removed += 1
print("Consolidated actors removed:", removed)

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

# verify
cnt = 0
for a in actor_sub.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH and abs(a.get_actor_location().z - 3380.434814) < 5.0:
        cnt += 1
print("SM_Floor_3 StaticMeshActors now at Z=3380:", cnt)
print("DONE")
