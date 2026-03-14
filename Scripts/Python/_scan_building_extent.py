import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Find building extent (all actors)
min_x, max_x = 99999, -99999
min_y, max_y = 99999, -99999
min_z, max_z = 99999, -99999
total = 0

for a in eas.get_all_level_actors():
    cn = a.get_class().get_name()
    # Skip sky/fog/post-process — not part of the building
    if cn in ("SkyAtmosphere", "ExponentialHeightFog", "PostProcessVolume", "DirectionalLight", "SkyLight"):
        continue
    loc = a.get_actor_location()
    min_x = min(min_x, loc.x)
    max_x = max(max_x, loc.x)
    min_y = min(min_y, loc.y)
    max_y = max(max_y, loc.y)
    min_z = min(min_z, loc.z)
    max_z = max(max_z, loc.z)
    total += 1

print(f"=== Building Extent ({total} actors) ===")
print(f"  X: {min_x:.0f} to {max_x:.0f} (width: {max_x-min_x:.0f})")
print(f"  Y: {min_y:.0f} to {max_y:.0f} (depth: {max_y-min_y:.0f})")
print(f"  Z: {min_z:.0f} to {max_z:.0f} (height: {max_z-min_z:.0f})")

# Find SM_OutsideFloor positions
print(f"\n=== SM_OutsideFloor actors ===")
for a in eas.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    comps = a.get_components_by_class(unreal.StaticMeshComponent)
    if not comps:
        continue
    sm = comps[0].get_editor_property("static_mesh")
    if sm and sm.get_name() == "SM_OutsideFloor":
        loc = a.get_actor_location()
        scale = a.get_actor_scale3d()
        rot = a.get_actor_rotation()
        # Get bounds
        bounds_min, bounds_max = comps[0].get_local_bounds()
        world_size_x = (bounds_max.x - bounds_min.x) * scale.x
        world_size_y = (bounds_max.y - bounds_min.y) * scale.y
        print(f"  {a.get_name()} at ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) rot=({rot.roll:.0f},{rot.pitch:.0f},{rot.yaw:.0f}) scale=({scale.x:.1f},{scale.y:.1f},{scale.z:.1f}) worldSize=({world_size_x:.0f}x{world_size_y:.0f})")
