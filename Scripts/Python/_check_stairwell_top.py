"""Find floor tiles used in the building and check for a roof surface."""
import unreal

ladder_x = -4548
ladder_y = -1600

subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = subsys.get_all_level_actors()

# 1. Find large flat meshes anywhere near the stairwell X,Y (within 2000 UU)
#    that could be floors at ANY Z level
print("=== LARGE FLAT MESHES (extent.z < 20, extent.x > 100 or extent.y > 100) near stairwell ===\n")
floors_by_z = {}
for actor in actors:
    if not isinstance(actor, unreal.StaticMeshActor):
        continue
    loc = actor.get_actor_location()
    if abs(loc.x - ladder_x) > 2000 or abs(loc.y - ladder_y) > 2000:
        continue
    origin, extent = actor.get_actor_bounds(False)
    if extent.z < 20 and (extent.x > 100 or extent.y > 100):
        name = actor.get_actor_label()
        z_key = round(loc.z / 50) * 50  # Group by ~50 UU Z bands
        if z_key not in floors_by_z:
            floors_by_z[z_key] = []
        # Get static mesh asset name
        smc = actor.static_mesh_component
        mesh_name = str(smc.static_mesh.get_name()) if smc and smc.static_mesh else "None"
        floors_by_z[z_key].append({
            'name': name,
            'mesh': mesh_name,
            'loc': (round(loc.x, 0), round(loc.y, 0), round(loc.z, 0)),
            'extent': (round(extent.x, 0), round(extent.y, 0), round(extent.z, 0))
        })

for z_key in sorted(floors_by_z.keys()):
    items = floors_by_z[z_key]
    print(f"  --- Z ≈ {z_key} ({len(items)} meshes) ---")
    for a in items[:5]:  # Show max 5 per Z level
        print(f"    {a['name']} mesh={a['mesh']} loc={a['loc']} extent={a['extent']}")
    if len(items) > 5:
        print(f"    ... and {len(items) - 5} more")

# 2. Check specifically what the "roof" level looks like
print(f"\n=== ALL ACTORS WITH 'floor' OR 'tile' OR 'roof' IN NAME (within 2000 UU) ===\n")
for actor in actors:
    name = actor.get_actor_label().lower()
    if not ('floor' in name or 'tile' in name or 'roof' in name or 'ground' in name):
        continue
    loc = actor.get_actor_location()
    if abs(loc.x - ladder_x) > 2000 or abs(loc.y - ladder_y) > 2000:
        continue
    origin, extent = actor.get_actor_bounds(False)
    print(f"  {actor.get_actor_label()} loc=({round(loc.x,0)}, {round(loc.y,0)}, {round(loc.z,0)}) extent=({round(extent.x,0)}, {round(extent.y,0)}, {round(extent.z,0)})")
