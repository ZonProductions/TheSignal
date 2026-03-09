import unreal

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()

# Get F1 stairs
stairs = None
for a in actors:
    if a.get_actor_label() == "SM_Steps":
        stairs = a
        break

if stairs:
    loc = stairs.get_actor_location()
    scale = stairs.get_actor_scale3d()
    # Mesh bounds: Z min=-1.45, Z max=375.93
    mesh_z_min = -1.45
    mesh_z_max = 375.93
    world_top = loc.z + (mesh_z_max * scale.z)
    world_bot = loc.z + (mesh_z_min * scale.z)
    print(f"Stairs: base Z={loc.z:.1f}, scale Z={scale.z:.4f}")
    print(f"Stairs world Z range: {world_bot:.1f} to {world_top:.1f}")

# Find the landing tiles nearest the F2 door (X~939, Y~-1324)
# and the F2 door itself
door_x, door_y = 939.4, -1324.0
print(f"\nF2 door at X={door_x}, Y={door_y}")

# Find closest floor-like actors to the door on F2
landing_candidates = []
for a in actors:
    label = a.get_actor_label()
    al = a.get_actor_location()
    # Look for tiles/cubes near the F2 door, Z between 470-520
    if al.z > 470 and al.z < 520:
        dx = abs(al.x - door_x)
        dy = abs(al.y - door_y)
        dist = (dx*dx + dy*dy)**0.5
        if dist < 400 and ("Floor" in label or "Cube" in label or "landing" in label.lower() or "platform" in label.lower()):
            # Get mesh bounds if static mesh
            cls = a.get_class().get_name()
            mesh_name = ""
            if cls == "StaticMeshActor":
                sm = a.static_mesh_component.static_mesh
                if sm:
                    mesh_name = sm.get_name()
                    bb = sm.get_bounding_box()
                    s = a.get_actor_scale3d()
                    top_z = al.z + (bb.max.z * s.z)
                    landing_candidates.append((label, al.z, top_z, dist, mesh_name, s.z))

landing_candidates.sort(key=lambda x: x[3])  # sort by distance
print(f"\nFloor surfaces near F2 door (within 400 UU):")
for label, z, top_z, dist, mesh, sz in landing_candidates[:10]:
    print(f"  {label}: Z={z:.1f}, top surface={top_z:.1f}, dist={dist:.0f}, mesh={mesh}, scaleZ={sz}")
