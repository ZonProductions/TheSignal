import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

delete_meshes = {"SM_ThinBoxVertical", "SM_ThinBoxHorizen", "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling"}

f1_f4 = 0
f5 = 0
by_mesh = {}

for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm:
        continue
    mn = sm.get_name()
    if mn not in delete_meshes:
        continue

    z = a.get_actor_location().z
    if z > 1900:
        f5 += 1
    else:
        f1_f4 += 1
        by_mesh[mn] = by_mesh.get(mn, 0) + 1

print(f"=== DELETION PLAN (F1-F4 only, F5 untouched) ===")
print(f"Total to delete: {f1_f4}")
for mn, c in sorted(by_mesh.items(), key=lambda x: -x[1]):
    print(f"  {c}x {mn}")
print(f"\nF5 safe (not touching): {f5}")
