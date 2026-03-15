import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

scales = {}
z_positions = {}

for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    loc = a.get_actor_location()
    if loc.z < 1900 or loc.z > 2300:
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if not sm or sm.get_name() != "SM_Cube":
        continue

    s = a.get_actor_scale3d()
    key = f"({s.x:.1f},{s.y:.1f},{s.z:.1f})"
    scales[key] = scales.get(key, 0) + 1

    z_key = round(loc.z)
    z_positions[z_key] = z_positions.get(z_key, 0) + 1

print(f"=== SM_Cube scale patterns on F5 ===")
for k, v in sorted(scales.items(), key=lambda x: -x[1])[:20]:
    print(f"  {v:4d}x scale={k}")

print(f"\n=== SM_Cube Z positions ===")
for z, v in sorted(z_positions.items()):
    print(f"  Z={z}: {v}")
