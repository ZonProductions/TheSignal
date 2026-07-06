import unreal

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if ues.get_editor_world() is None:
    raise Exception("ABORT: editor world None (PIE). Stop PIE first.")

TARGET_Z = 3380.434814
TOL = 5.0
LABEL = "Roof_Consolidated_Z3380"

deleted_consolidated = 0
deleted_floors = 0
for a in list(actor_sub.get_all_level_actors()):
    if a.get_actor_label() == LABEL:
        actor_sub.destroy_actor(a)
        deleted_consolidated += 1
        continue
    if a.get_class().get_name() == "StaticMeshActor":
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            sm = smc.get_editor_property('static_mesh')
            if sm and "SM_Floor" in sm.get_name() and abs(a.get_actor_location().z - TARGET_Z) < TOL:
                actor_sub.destroy_actor(a)
                deleted_floors += 1

print("Deleted consolidated roof actor(s):", deleted_consolidated)
print("Deleted individual floor actor(s) at Z~3380.43:", deleted_floors)

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

# verify nothing left at that Z
remaining = 0
for a in actor_sub.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        remaining += 1
    elif a.get_class().get_name() == "StaticMeshActor":
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            sm = smc.get_editor_property('static_mesh')
            if sm and "SM_Floor" in sm.get_name() and abs(a.get_actor_location().z - TARGET_Z) < TOL:
                remaining += 1
print("Remaining floor/roof actors at Z=3380.434814:", remaining)
print("DONE")
