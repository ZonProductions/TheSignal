"""Replace large floor slabs with 1x1m tile grid for level design carving.

Run this script repeatedly. Each run spawns a batch of columns.
When all columns are done, it deletes the original floor slabs.

Set FLOOR_NUM to control which floor to tile.
"""
import unreal

# === CONFIGURATION ===
FLOOR_NUM = 1        # Which floor to tile (1-5)
TILE_SIZE = 100      # 100 UU = 1 meter
FLOOR_HEIGHT = 500
COLS_PER_RUN = 5     # Columns per batch

# === FLOOR SLAB DATA ===
SLAB_Z = -12.87
SLAB_Z_SCALE = 0.28
FLOOR_MATERIAL_PATH = '/Game/office_BigCompanyArchViz/Materials/MI_Floor.MI_Floor'
CUBE_MESH_PATH = '/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_Cube.SM_Cube'

GRID_MIN_X = -4800
GRID_MAX_X = 2200
GRID_MIN_Y = -1800
GRID_MAX_Y = 1400

# ======================

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ls = unreal.get_editor_subsystem(unreal.LayersSubsystem)

z_offset = (FLOOR_NUM - 1) * FLOOR_HEIGHT
prefix = f"FloorTile_F{FLOOR_NUM}_"

cols = int((GRID_MAX_X - GRID_MIN_X) / TILE_SIZE)
rows = int((GRID_MAX_Y - GRID_MIN_Y) / TILE_SIZE)
half_tile = TILE_SIZE / 2.0
tile_scale_xy = TILE_SIZE / 100.0  # 1.0 for 1m tiles

# Check progress
all_actors = eas.get_all_level_actors()
existing_cols = set()
for a in all_actors:
    label = a.get_actor_label()
    if label.startswith(prefix):
        parts = label.split('_')
        if len(parts) >= 4:
            existing_cols.add(int(parts[2]))

next_col = 0
while next_col in existing_cols:
    next_col += 1

if next_col >= cols:
    fpfx = f"F{FLOOR_NUM}_" if FLOOR_NUM > 1 else ""
    targets = [f"{fpfx}SM_Cube9", f"{fpfx}SM_Cube10"]
    deleted = 0
    for a in all_actors:
        if a.get_actor_label() in targets:
            scale = a.get_actor_scale3d()
            if scale.x > 10 and scale.y > 10 and scale.z < 1:
                eas.destroy_actor(a)
                deleted += 1
    print(f"All {cols} columns done! Deleted {deleted} original floor slabs.")
    print(f"Floor {FLOOR_NUM}: {cols * rows} tiles total.")
    raise SystemExit

cube_mesh = unreal.load_asset(CUBE_MESH_PATH)
floor_mat = unreal.load_asset(FLOOR_MATERIAL_PATH)
sma_class = unreal.StaticMeshActor.static_class()

end_col = min(next_col + COLS_PER_RUN, cols)
spawned = 0

print(f"Floor {FLOOR_NUM}: columns {next_col}-{end_col-1} of {cols-1} ({rows} tiles each)")

for col in range(next_col, end_col):
    x = GRID_MIN_X + half_tile + col * TILE_SIZE
    for row in range(rows):
        y = GRID_MIN_Y + half_tile + row * TILE_SIZE
        z = SLAB_Z + z_offset

        tile = eas.spawn_actor_from_class(sma_class, unreal.Vector(x, y, z))
        if not tile:
            continue

        tile.set_actor_scale3d(unreal.Vector(tile_scale_xy, tile_scale_xy, SLAB_Z_SCALE))
        comp = tile.get_components_by_class(unreal.StaticMeshComponent)[0]
        comp.set_static_mesh(cube_mesh)
        comp.set_material(0, floor_mat)
        tile.set_actor_label(f"FloorTile_F{FLOOR_NUM}_{col:02d}_{row:02d}")
        ls.add_actor_to_layer(tile, f"Floor_{FLOOR_NUM}")
        spawned += 1

remaining = cols - end_col
print(f"Spawned {spawned} tiles. {remaining} columns remaining.")
if remaining > 0:
    print("Run again to continue.")
else:
    print("All columns done! Run once more to delete original slabs.")
