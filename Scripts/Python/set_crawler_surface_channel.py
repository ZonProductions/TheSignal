"""
Set CrawlerSurface collision channel on level meshes.

Structural surfaces (walls, floors, ceilings) BLOCK the CrawlerSurface channel.
Furniture/props IGNORE it — invisible to BPC_Climbing's surface traces.

Run after placing new levels or adding furniture.
Usage: exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/set_crawler_surface_channel.py').read())
"""
import unreal

# ECC_GameTraceChannel1 = CrawlerSurface (defined in DefaultEngine.ini)
CRAWLER_CHANNEL = unreal.CollisionChannel.ECC_CRAWLER_SURFACE

# Paths that indicate PROPS/FURNITURE — set to IGNORE
PROP_PATH_KEYWORDS = [
    "/Probs/",          # Office pack props folder
    "/Furniture/",
    "/Plants/",
    "/Decor/",
    "/Props/",
    "/Clutter/",
    "/Electronics/",
    "/Books/",
    "/Paper/",
]

# Paths that indicate STRUCTURAL — keep as BLOCK (default)
STRUCTURAL_PATH_KEYWORDS = [
    "/Environment/",    # Office pack structural folder
    "/Walls/",
    "/Floors/",
    "/Ceiling/",
    "/Architecture/",
    "FloorTile_F",      # Custom floor tiles
    "/TreatmentStation/",
]

subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = subsystem.get_all_level_actors()

ignore_count = 0
block_count = 0
skip_count = 0

for actor in all_actors:
    # Only process static mesh actors
    sm_comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not sm_comp:
        continue

    mesh = sm_comp.get_editor_property("static_mesh")
    if not mesh:
        continue

    mesh_path = mesh.get_path_name()

    # Check if this is a prop/furniture
    is_prop = False
    for keyword in PROP_PATH_KEYWORDS:
        if keyword.lower() in mesh_path.lower():
            is_prop = True
            break

    # Check if this is structural (overrides prop classification)
    is_structural = False
    for keyword in STRUCTURAL_PATH_KEYWORDS:
        if keyword.lower() in mesh_path.lower():
            is_structural = True
            break

    if is_structural:
        # Already blocks by default — no change needed
        block_count += 1
    elif is_prop:
        # Set to IGNORE so BPC_Climbing can't see it
        sm_comp.set_collision_response_to_channel(CRAWLER_CHANNEL, unreal.CollisionResponseType.ECR_IGNORE)
        ignore_count += 1
    else:
        # Unknown — keep default (Block). Better to climb a mystery mesh than fall through a floor.
        skip_count += 1

print(f"[CrawlerSurface] Done: {ignore_count} props set to IGNORE, {block_count} structural confirmed, {skip_count} unknown (kept as Block)")
