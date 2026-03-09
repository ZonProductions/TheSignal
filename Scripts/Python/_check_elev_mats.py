"""Compare SM_ElevatorWall material elements across all floors."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

walls = []
for a in all_actors:
    label = a.get_actor_label()
    if 'ElevatorWall' in label:
        walls.append(a)

walls.sort(key=lambda a: a.get_actor_location().z)

for a in walls:
    label = a.get_actor_label()
    loc = a.get_actor_location()
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc:
        continue

    sm = smc.get_editor_property("static_mesh")
    mesh_name = sm.get_name() if sm else "None"

    # Get mesh material slot count from the mesh asset
    mesh_mat_count = sm.get_num_sections(0) if sm else 0  # LOD 0

    # Get component material count (includes overrides)
    comp_mat_count = smc.get_num_materials()

    print(f"\n=== {label} (Z={loc.z:.0f}) ===")
    print(f"  Mesh: {mesh_name}, Mesh sections(LOD0): {mesh_mat_count}, Component materials: {comp_mat_count}")

    for i in range(max(comp_mat_count, 10)):  # Check up to 10 slots
        try:
            mat = smc.get_material(i)
            if mat:
                mat_name = mat.get_name()
                # Check if it's an override
                override = smc.get_editor_property('override_materials')
                print(f"  Slot {i}: {mat_name}")
            else:
                if i < comp_mat_count:
                    print(f"  Slot {i}: None")
        except:
            break

    # Also check override_materials array
    try:
        overrides = smc.get_editor_property('override_materials')
        if overrides:
            print(f"  Override materials array: {len(overrides)} entries")
            for i, ov in enumerate(overrides):
                if ov:
                    print(f"    Override[{i}]: {ov.get_name()}")
                else:
                    print(f"    Override[{i}]: None (uses mesh default)")
    except Exception as e:
        print(f"  Could not read override_materials: {e}")

    # Check all mesh material slot names
    if sm:
        try:
            for i in range(comp_mat_count):
                slot_name = sm.get_material_slot_names()
                if i == 0:
                    print(f"  Mesh slot names: {[str(n) for n in slot_name]}")
        except:
            pass
