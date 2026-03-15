import unreal
import csv
import os

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

FLOOR_Z_MIN = 1900
FLOOR_Z_MAX = 2300
FLOOR_LEVEL_Z = 1987
CELL_SIZE = 100
OUTPUT_PATH = "C:/Users/Ommei/workspace/TheSignal/Scripts/FloorPlans/F5_v3.csv"

wall_meshes = {
    "SM_Cube", "SM_Column", "SM_Cylinder",
    "SM_WindowWall", "SM_WcWall", "SM_Securitywall",
    "SM_FrameTall", "SM_FrameTallDoor", "SM_LastLineEndWall",
    "SM_CenterRoomsinnerWall", "SM_ElevatorWall",
}

door_meshes = {
    "SM_DoorOffice", "SM_DoorOfficeFrame",
    "SM_DoorExit", "SM_DoorExitFrame",
    "SM_AutoDoorBase", "SM_AutoDoorLeft", "SM_AutoDoorRight",
    "SM_RoomManagerDoor",
}

door_bp_classes = {"BP_WCDoor01_C", "BP_WCDoor02_C", "BP_GlassDoors1_C", "BP_ElevatorDoors_C", "ZP_InteractDoor"}

# Collect wall and door BOUNDS (not just origin points)
wall_bounds = []  # (x_min, y_min, x_max, y_max)
door_bounds = []

def get_xy_bounds(actor, comp):
    """Get world-space XY bounding box of a mesh component."""
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    bmin, bmax = comp.get_local_bounds()
    # World bounds
    wx_min = loc.x + bmin.x * abs(scale.x)
    wx_max = loc.x + bmax.x * abs(scale.x)
    wy_min = loc.y + bmin.y * abs(scale.y)
    wy_max = loc.y + bmax.y * abs(scale.y)
    # Handle negative scales (mirrored building)
    if wx_min > wx_max:
        wx_min, wx_max = wx_max, wx_min
    if wy_min > wy_max:
        wy_min, wy_max = wy_max, wy_min
    return (wx_min, wy_min, wx_max, wy_max)

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < FLOOR_Z_MIN or loc.z > FLOOR_Z_MAX:
        continue

    cn = a.get_class().get_name()

    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm:
            continue
        mn = sm.get_name()

        if mn == "SM_Cube":
            s = a.get_actor_scale3d()
            is_floor_tile = (abs(s.x - 1.0) < 0.1 and abs(s.y - 1.0) < 0.1 and s.z < 0.5 and abs(loc.z - FLOOR_LEVEL_Z) < 20)
            if is_floor_tile:
                continue
            wall_bounds.append(get_xy_bounds(a, comps[0]))
        elif mn in wall_meshes:
            wall_bounds.append(get_xy_bounds(a, comps[0]))
        elif mn in door_meshes:
            door_bounds.append(get_xy_bounds(a, comps[0]))

    elif cn in door_bp_classes:
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            door_bounds.append(get_xy_bounds(a, comps[0]))
        else:
            # Fallback: use actor location as a single cell
            door_bounds.append((loc.x - 50, loc.y - 50, loc.x + 50, loc.y + 50))

print(f"F5 scan: {len(wall_bounds)} wall actors, {len(door_bounds)} door actors")

# Find global extent
all_bounds = wall_bounds + door_bounds
global_x_min = min(b[0] for b in all_bounds)
global_x_max = max(b[2] for b in all_bounds)
global_y_min = min(b[1] for b in all_bounds)
global_y_max = max(b[3] for b in all_bounds)

grid_cols = int((global_x_max - global_x_min) / CELL_SIZE) + 2
grid_rows = int((global_y_max - global_y_min) / CELL_SIZE) + 2

print(f"Grid: {grid_cols} cols x {grid_rows} rows")
print(f"World: X {global_x_min:.0f}-{global_x_max:.0f}, Y {global_y_min:.0f}-{global_y_max:.0f}")

grid = [['.' for _ in range(grid_cols)] for _ in range(grid_rows)]

def fill_cells(bounds_list, char):
    for x_min, y_min, x_max, y_max in bounds_list:
        col_start = max(0, int((x_min - global_x_min) / CELL_SIZE))
        col_end = min(grid_cols - 1, int((x_max - global_x_min) / CELL_SIZE))
        row_start = max(0, int((y_min - global_y_min) / CELL_SIZE))
        row_end = min(grid_rows - 1, int((y_max - global_y_min) / CELL_SIZE))
        for r in range(row_start, row_end + 1):
            for c in range(col_start, col_end + 1):
                grid[r][c] = char

# Walls first, then doors overwrite
fill_cells(wall_bounds, 'X')
fill_cells(door_bounds, '}')

os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
with open(OUTPUT_PATH, 'w', newline='') as f:
    writer = csv.writer(f)
    for r in range(grid_rows):
        writer.writerow(grid[r])

wall_count = sum(row.count('X') for row in grid)
door_count = sum(row.count('}') for row in grid)
empty_count = sum(row.count('.') for row in grid)
print(f"Cells: {wall_count} walls, {door_count} doors, {empty_count} empty")
print(f"Saved: {OUTPUT_PATH}")
