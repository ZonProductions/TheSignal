"""Add a floor mesh at the roof level around the stairwell ladder opening.
The fences are at Z=2612.5, so the floor surface should be at Z≈2612.
Use SM_Cube (same as other floor sections in the building)."""
import unreal

# The roof floor Z — fence bases are at 2612.5
FLOOR_Z = 2612.0

# Stairwell center (ladder area)
STAIRWELL_X = -4500.0
STAIRWELL_Y = -1600.0

# Floor tile size — large enough to cover the stairwell landing area
# SM_Cube is 200x200x10 UU by default, we'll scale it
FLOOR_MESH_PATH = "/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_Cube.SM_Cube"

# Load the mesh
floor_mesh = unreal.load_asset(FLOOR_MESH_PATH)
if not floor_mesh:
    print("ERROR: Could not find SM_Cube mesh!")
else:
    print(f"Found floor mesh: {floor_mesh.get_name()}")

    # Spawn the floor actor
    subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Place floor tile - covers area around stairwell
    # Position it to cover the landing area where the player exits
    loc = unreal.Vector(STAIRWELL_X, STAIRWELL_Y, FLOOR_Z)
    rot = unreal.Rotator(0, 0, 0)

    actor = subsys.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if actor:
        smc = actor.static_mesh_component
        smc.set_static_mesh(floor_mesh)
        # Scale to cover stairwell area (~400x400 UU, thin)
        # SM_Cube base is 100x100x100. Scale X/Y=4 gives 400 UU, Z=0.1 gives 10 UU thick
        actor.set_actor_scale3d(unreal.Vector(4.0, 4.0, 0.1))
        actor.set_actor_label("F5_StairwellFloor_Roof")

        # Check final bounds
        origin, extent = actor.get_actor_bounds(False)
        print(f"Placed floor: {actor.get_actor_label()}")
        print(f"  Location: ({loc.x}, {loc.y}, {loc.z})")
        print(f"  Bounds center: ({round(origin.x,1)}, {round(origin.y,1)}, {round(origin.z,1)})")
        print(f"  Extent: ({round(extent.x,1)}, {round(extent.y,1)}, {round(extent.z,1)})")
        print(f"  Z range: [{round(origin.z - extent.z, 1)}, {round(origin.z + extent.z, 1)}]")
    else:
        print("ERROR: Failed to spawn actor")
