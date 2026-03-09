"""Exhaustive search: every actor near the elevator wall corridor on F5.
Elevator wall Y=-1316, spans X=360 to X=1738. Search wide."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# F5 elevator wall: Y=-1316, X=360-1738, Z=1999-2399
# Search the entire corridor width, generous Y range
Y_MIN, Y_MAX = -1380, -1250
Z_MIN, Z_MAX = 1900, 2500
X_MIN, X_MAX = 300, 1800

print("=== EVERY actor in F5 elevator wall corridor ===")
print(f"=== X({X_MIN}-{X_MAX}) Y({Y_MIN}-{Y_MAX}) Z({Z_MIN}-{Z_MAX}) ===\n")

found = []
for a in all_actors:
    loc = a.get_actor_location()
    if not (X_MIN <= loc.x <= X_MAX and Y_MIN <= loc.y <= Y_MAX and Z_MIN <= loc.z <= Z_MAX):
        continue

    cls = a.get_class().get_name()
    label = a.get_actor_label()
    mesh_name = ""
    mat_list = ""

    # Try to get mesh info from any component
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        sm = smc.get_editor_property("static_mesh")
        if sm:
            mesh_name = sm.get_name()
        mats = []
        for i in range(smc.get_num_materials()):
            mat = smc.get_material(i)
            if mat:
                mats.append(mat.get_name())
        mat_list = ", ".join(mats)

    # Check for attached actors
    attached = a.get_attached_actors()
    attach_info = ""
    if attached and len(attached) > 0:
        attach_info = f" | ATTACHED: {[x.get_actor_label() for x in attached]}"

    parent = a.get_attach_parent_actor()
    parent_info = ""
    if parent:
        parent_info = f" | PARENT: {parent.get_actor_label()}"

    found.append((label, cls, loc, mesh_name, mat_list, attach_info, parent_info))

found.sort(key=lambda x: x[2].x)
for label, cls, loc, mesh, mats, attach, parent in found:
    print(f"  {label:50s} | {cls:25s} | {mesh:25s} | ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) | {mats}{attach}{parent}")

print(f"\nTotal: {len(found)}")

# Also check: what actors exist on F5 near elevator but NOT on F1?
print("\n\n=== Checking F1 equivalent area ===")
f1_labels = set()
f5_labels = set()
for a in all_actors:
    loc = a.get_actor_location()
    if X_MIN <= loc.x <= X_MAX and Y_MIN <= loc.y <= Y_MAX:
        label = a.get_actor_label()
        if -100 <= loc.z <= 500:
            f1_labels.add(label)
        elif 1900 <= loc.z <= 2500:
            f5_labels.add(label)

# Strip floor prefixes for comparison
def base_name(label):
    for prefix in ['F5_', 'F4_', 'F3_', 'F2_']:
        if label.startswith(prefix):
            return label[3:]
    return label

f1_bases = {base_name(l) for l in f1_labels}
print(f"F1 actors in area: {len(f1_labels)}")
print(f"F5 actors in area: {len(f5_labels)}")
print(f"\nF5-only (no F1 equivalent):")
for label in sorted(f5_labels):
    bn = base_name(label)
    if bn not in f1_bases and label not in f1_labels:
        # Get the actor details
        for a in all_actors:
            if a.get_actor_label() == label:
                cls = a.get_class().get_name()
                loc = a.get_actor_location()
                smc = a.get_component_by_class(unreal.StaticMeshComponent)
                mesh = ""
                if smc:
                    sm = smc.get_editor_property("static_mesh")
                    if sm:
                        mesh = sm.get_name()
                print(f"  {label:50s} | {cls:25s} | {mesh:25s} | ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f})")
                break
