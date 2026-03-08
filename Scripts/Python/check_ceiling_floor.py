"""Check ceiling/slab geometry to find exact floor-to-floor height."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Find ceiling-height actors on Floor 1 (Z > 300, no F2/F3/F4/F5 prefix)
print("=== Floor 1 actors above Z=300 (ceiling area) ===")
high_f1 = []
for a in all_actors:
    label = a.get_actor_label()
    if label.startswith(('F2_', 'F3_', 'F4_', 'F5_')):
        continue
    z = a.get_actor_location().z
    if 300 < z < 600:
        high_f1.append((z, label, a.get_class().get_name()))

high_f1.sort()
for z, name, cls in high_f1:
    print(f"  Z={z:7.1f}: {name} ({cls})")

# Find lowest Floor 2 actors
print("\n=== Floor 2 actors below Z=550 (ground area) ===")
low_f2 = []
for a in all_actors:
    label = a.get_actor_label()
    if not label.startswith('F2_'):
        continue
    z = a.get_actor_location().z
    if z < 550:
        low_f2.append((z, label, a.get_class().get_name()))

low_f2.sort()
for z, name, cls in low_f2[:30]:
    print(f"  Z={z:7.1f}: {name} ({cls})")

# Check mesh bounds of floor and ceiling meshes using trace
print("\n=== Checking mesh extents via component bounds ===")
for a in all_actors:
    label = a.get_actor_label()
    if label in ('Guest_floor', 'F2_Guest_floor2'):
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            # Get local bounds
            origin, extent = comps[0].get_local_bounds()
            loc = a.get_actor_location()
            print(f"  {label}: actor_z={loc.z:.1f}, bounds_origin_z={origin.z:.1f}, bounds_extent_z={extent.z:.1f}")
            print(f"    -> mesh goes from Z={loc.z + origin.z - extent.z:.1f} to Z={loc.z + origin.z + extent.z:.1f}")
