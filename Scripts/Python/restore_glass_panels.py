"""Restore glass window panels to F1-F4 from F5 references.
Also fixes flush alignment with the wall on all floors including F5."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Find the F5 glass panels
f5_panels = []
for a in all_actors:
    label = a.get_actor_label()
    if label in ('F5_line_box_glass3', 'F5_line_box_glass4'):
        f5_panels.append(a)
        loc = a.get_actor_location()
        scale = a.get_actor_scale3d()
        rot = a.get_actor_rotation()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mat = smc.get_material(0) if smc else None
        print(f"Found: {label}")
        print(f"  Pos: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
        print(f"  Scale: ({scale.x:.3f}, {scale.y:.3f}, {scale.z:.3f})")
        print(f"  Rot: (R={rot.roll:.1f}, P={rot.pitch:.1f}, Y={rot.yaw:.1f})")

if len(f5_panels) != 2:
    print(f"ERROR: Expected 2 panels, found {len(f5_panels)}")

# F5 base Z = 2000, floor offset = 500 per floor
# F1=0, F2=500, F3=1000, F4=1500, F5=2000
F5_BASE_Z = 2000.0
FLOOR_OFFSETS = {1: 0.0, 2: 500.0, 3: 1000.0, 4: 1500.0}

# Flush fix: move panels 3.5 UU south (more negative Y) to align with wall face
FLUSH_Y_ADJUSTMENT = -3.5

# Load the mesh and material
glass_mesh = unreal.load_asset("/Game/BigCompanyOffice/Meshes/SM_FireExtinguisherBoxGlass")
glass_mat = unreal.load_asset("/Game/BigCompanyOffice/Materials/MI_Glass_AO")

if not glass_mesh:
    print("ERROR: Could not load SM_FireExtinguisherBoxGlass mesh")
if not glass_mat:
    print("ERROR: Could not load MI_Glass_AO material")

created = 0

# Fix F5 panels flush first
for panel in f5_panels:
    loc = panel.get_actor_location()
    new_y = loc.y + FLUSH_Y_ADJUSTMENT
    panel.set_actor_location(unreal.Vector(loc.x, new_y, loc.z), False, False)
    print(f"Fixed flush: {panel.get_actor_label()} Y: {loc.y:.1f} -> {new_y:.1f}")

# Create panels on F1-F4
for floor_num, floor_z_base in FLOOR_OFFSETS.items():
    z_offset = floor_z_base - F5_BASE_Z  # negative offset from F5

    for panel in f5_panels:
        loc = panel.get_actor_location()  # Already flush-fixed
        scale = panel.get_actor_scale3d()
        rot = panel.get_actor_rotation()

        # Calculate position for this floor
        new_z = loc.z + z_offset
        new_loc = unreal.Vector(loc.x, loc.y, new_z)

        # Spawn new actor
        new_actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, new_loc)
        if not new_actor:
            print(f"  FAILED to spawn on F{floor_num}")
            continue

        # Configure
        smc = new_actor.get_component_by_class(unreal.StaticMeshComponent)
        if smc and glass_mesh:
            smc.set_static_mesh(glass_mesh)
            if glass_mat:
                smc.set_material(0, glass_mat)

        new_actor.set_actor_rotation(rot, False)
        new_actor.set_actor_scale3d(scale)

        # Name it
        old_label = panel.get_actor_label()
        # F5_line_box_glass3 -> F{n}_line_box_glass3
        base_name = old_label.replace('F5_', '')
        new_label = f"F{floor_num}_{base_name}"
        new_actor.set_actor_label(new_label)

        new_loc_check = new_actor.get_actor_location()
        print(f"  Created: {new_label} at ({new_loc_check.x:.0f}, {new_loc_check.y:.0f}, {new_loc_check.z:.0f})")
        created += 1

print(f"\nDone: {created} glass panels created across F1-F4")
print(f"F5 panels repositioned for flush alignment")
print("Remember to Ctrl+S to save!")
