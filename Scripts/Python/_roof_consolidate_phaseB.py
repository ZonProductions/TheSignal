import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ROOF_MESH = "/Game/ResearchMegaPack/ResearchFacility/Meshes/SM_Floor_3.SM_Floor_3"
TARGET_Z = 3380.434814
TOL = 5.0
LABEL = "Roof_Consolidated_Z3380"

# safety: confirm the consolidated actor exists with 845 instances BEFORE deleting anything
consolidated = None
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        consolidated = a
        break
if not consolidated:
    raise Exception("ABORT: consolidated actor missing - refusing to delete originals.")
hism = consolidated.get_components_by_class(unreal.InstancedStaticMeshComponent)[0]
if hism.get_instance_count() != 845:
    raise Exception("ABORT: expected 845 instances, found %d" % hism.get_instance_count())
print("Guard OK: consolidated actor present with", hism.get_instance_count(), "instances.")

# collect originals (identical filter as Phase A)
victims = []
for a in actor_sub.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue
    sm = smc.get_editor_property('static_mesh')
    if sm and sm.get_path_name() == ROOF_MESH and abs(a.get_actor_location().z - TARGET_Z) < TOL:
        victims.append(a)

print("Original tiles matched for deletion:", len(victims))
if len(victims) != 845:
    raise Exception("ABORT: expected 845 originals, found %d" % len(victims))

deleted = 0
for a in victims:
    if actor_sub.destroy_actor(a):
        deleted += 1
print("Deleted:", deleted)

# save
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

# final integrity check
remaining = [a for a in actor_sub.get_all_level_actors()
             if a.get_class().get_name() == "StaticMeshActor"
             and a.get_component_by_class(unreal.StaticMeshComponent)
             and a.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh')
             and a.get_component_by_class(unreal.StaticMeshComponent).get_editor_property('static_mesh').get_path_name() == ROOF_MESH
             and abs(a.get_actor_location().z - TARGET_Z) < TOL]
print("Remaining individual SM_Floor_3 tiles at roof Z:", len(remaining))
print("Consolidated HISM instance count:", hism.get_instance_count())
print("DONE")
