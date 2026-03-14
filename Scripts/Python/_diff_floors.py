import unreal
from collections import defaultdict

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Already hidden meshes (skip these)
already_hidden = {
    "SM_ThinBoxVertical", "SM_ThinBoxHorizen",
    "SM_ConferenceSecretaryRoom", "SM_PlayRoomFloorSilling",
    "SM_DoorOffice", "SM_DoorOfficeFrame",
}

# Count meshes per floor (F1 vs F2)
f1_meshes = defaultdict(int)
f2_meshes = defaultdict(int)

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
    if mn in already_hidden:
        continue

    z = a.get_actor_location().z
    if -50 < z < 200:
        f1_meshes[mn] += 1
    elif 400 < z < 700:
        f2_meshes[mn] += 1

# Find meshes on F2 but missing/reduced on F1
print("=== Meshes on F2 but MISSING or REDUCED on F1 ===")
print("(These are what you manually removed from F1)\n")

candidates = []
for mn, f2_count in sorted(f2_meshes.items(), key=lambda x: -x[1]):
    f1_count = f1_meshes.get(mn, 0)
    if f1_count < f2_count:
        diff = f2_count - f1_count
        candidates.append((mn, f1_count, f2_count, diff))

for mn, f1, f2, diff in candidates:
    status = "GONE" if f1 == 0 else f"reduced {f2} → {f1}"
    print(f"  {diff:4d}x {mn}  (F1={f1}, F2={f2}) {status}")

print(f"\nTotal mesh types missing/reduced: {len(candidates)}")
print(f"Total actors to add to delete list: {sum(c[3] for c in candidates)}")
