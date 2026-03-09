"""
generate_floor_plan.py — Generate RE-style floor plan texture from line traces.

Scans the level at player height using a dense grid of downward traces.
Open spaces (trace hits floor) vs solid geometry (wall blocks trace).
Renders wall outlines as white lines on a dark background.

Usage: Run via MCP Python endpoint:
  exec(open('C:/Users/Ommei/workspace/TheSignal/Scripts/Python/generate_floor_plan.py').read())
"""
import unreal
import struct
import os

# ── Config ────────────────────────────────────────────────────
AREA_ID = 'BigCompany_Main'
CELL_SIZE = 25.0       # UU per grid cell (~25cm resolution)
TRACE_DROP = 600.0     # Max downward trace distance from scan height
OUTPUT_DIR = 'C:/Users/Ommei/workspace/TheSignal/Saved/MapCaptures'
CONTENT_PATH = '/Game/TheSignal/Textures/Maps'

# Floor plan colors (R, G, B)
COL_FLOOR = (25, 30, 45)        # Dark blue-grey (explored room)
COL_WALL  = (190, 200, 210)     # Light grey wall outlines
COL_VOID  = (5, 5, 8)           # Near-black void/exterior


# ── Find MapVolume ────────────────────────────────────────────
volume = None
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if 'ZP_MapVolume' in actor.get_class().get_name():
        if str(actor.get_editor_property('area_id')) == AREA_ID:
            volume = actor
            break

if not volume:
    raise RuntimeError(f'MapVolume "{AREA_ID}" not found')

center = volume.get_actor_location()
bounds_comp = None
for c in volume.get_components_by_class(unreal.BoxComponent):
    bounds_comp = c
    break
if not bounds_comp:
    raise RuntimeError('No BoxComponent on MapVolume')

ext = bounds_comp.get_scaled_box_extent()
x0, x1 = center.x - ext.x, center.x + ext.x
y0, y1 = center.y - ext.y, center.y + ext.y

cols = int((x1 - x0) / CELL_SIZE) + 1
rows = int((y1 - y0) / CELL_SIZE) + 1

# Scan from volume center Z (should be at ~player height inside rooms)
scan_z = center.z

print(f'[FloorPlan] Area: {x1-x0:.0f} x {y1-y0:.0f} UU')
print(f'[FloorPlan] Grid: {cols}x{rows} = {cols*rows} cells')
print(f'[FloorPlan] Scan Z: {scan_z:.0f}, drop: {TRACE_DROP:.0f}')


# ── Test trace API ────────────────────────────────────────────
test = unreal.SystemLibrary.line_trace_single(
    volume,
    unreal.Vector(x=center.x, y=center.y, z=scan_z),
    unreal.Vector(x=center.x, y=center.y, z=scan_z - TRACE_DROP),
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
    True, [], unreal.DrawDebugTrace.NONE, True)

is_tuple = isinstance(test, (tuple, list))
print(f'[FloorPlan] Test trace: type={type(test).__name__}, tuple={is_tuple}')
if is_tuple:
    print(f'  hit={test[0]}')
else:
    print(f'  hit={test}')


# ── Line Trace Grid ──────────────────────────────────────────
print('[FloorPlan] Scanning...')

grid = []  # grid[row][col] = True (open/floor) or False (solid/void)
channel = unreal.TraceTypeQuery.TRACE_TYPE_QUERY1

for row in range(rows):
    y = y0 + (row + 0.5) * CELL_SIZE
    row_data = []
    for col in range(cols):
        x = x0 + (col + 0.5) * CELL_SIZE

        start = unreal.Vector(x=x, y=y, z=scan_z)
        end = unreal.Vector(x=x, y=y, z=scan_z - TRACE_DROP)

        result = unreal.SystemLibrary.line_trace_single(
            volume, start, end, channel,
            True, [], unreal.DrawDebugTrace.NONE, True)

        if isinstance(result, (tuple, list)):
            hit = bool(result[0])
        else:
            hit = bool(result)

        row_data.append(hit)
    grid.append(row_data)

    if (row + 1) % 40 == 0 or row == rows - 1:
        print(f'  {row+1}/{rows} rows')

open_count = sum(sum(r) for r in grid)
print(f'[FloorPlan] Trace done. Open: {open_count}, Solid: {cols*rows - open_count}')


# ── Render Floor Plan ─────────────────────────────────────────
print('[FloorPlan] Rendering...')

def at_boundary(r, c):
    """Check if cell (r,c) is at a boundary between OPEN and SOLID."""
    val = grid[r][c]
    for dr, dc in ((-1,0),(1,0),(0,-1),(0,1)):
        nr, nc = r + dr, c + dc
        if nr < 0 or nr >= rows or nc < 0 or nc >= cols:
            # Grid edge: if cell is open, it borders void
            if val:
                return True
            continue
        if grid[nr][nc] != val:
            return True
    return False

pixels = []
for row in range(rows):
    for col in range(cols):
        is_open = grid[row][col]
        boundary = at_boundary(row, col)

        if boundary:
            pixels.append(COL_WALL)
        elif is_open:
            pixels.append(COL_FLOOR)
        else:
            pixels.append(COL_VOID)

print(f'[FloorPlan] Image: {cols}x{rows} pixels')


# ── Write BMP ─────────────────────────────────────────────────
os.makedirs(OUTPUT_DIR, exist_ok=True)
tex_name = f'T_Map_{AREA_ID}'
bmp_path = os.path.join(OUTPUT_DIR, f'{tex_name}.bmp').replace('\\', '/')

row_stride = (cols * 3 + 3) & ~3
data_size = row_stride * rows
file_size = 54 + data_size

with open(bmp_path, 'wb') as f:
    # BMP file header (14 bytes)
    f.write(b'BM')
    f.write(struct.pack('<I', file_size))
    f.write(struct.pack('<HH', 0, 0))
    f.write(struct.pack('<I', 54))
    # BITMAPINFOHEADER (40 bytes)
    f.write(struct.pack('<I', 40))
    f.write(struct.pack('<i', cols))
    f.write(struct.pack('<i', rows))
    f.write(struct.pack('<HH', 1, 24))
    f.write(struct.pack('<I', 0))       # compression
    f.write(struct.pack('<I', data_size))
    f.write(struct.pack('<i', 2835))    # X pixels/meter
    f.write(struct.pack('<i', 2835))    # Y pixels/meter
    f.write(struct.pack('<I', 0))
    f.write(struct.pack('<I', 0))
    # Pixel data (BMP stores bottom-to-top, BGR order)
    for r in range(rows - 1, -1, -1):
        base = r * cols
        for c in range(cols):
            rv, gv, bv = pixels[base + c]
            f.write(struct.pack('BBB', bv, gv, rv))
        pad = row_stride - cols * 3
        if pad > 0:
            f.write(b'\x00' * pad)

print(f'[FloorPlan] Wrote: {bmp_path}')


# ── Import into UE5 ──────────────────────────────────────────
task = unreal.AssetImportTask()
task.set_editor_property('filename', bmp_path)
task.set_editor_property('destination_path', CONTENT_PATH)
task.set_editor_property('destination_name', tex_name)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)
task.set_editor_property('replace_existing', True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
print(f'[FloorPlan] Imported: {CONTENT_PATH}/{tex_name}')

# Assign texture to the MapVolume
tex = unreal.load_asset(f'{CONTENT_PATH}/{tex_name}')
if tex:
    volume.set_editor_property('map_texture', tex)
    unreal.EditorLevelLibrary.save_current_level()
    print('[FloorPlan] Assigned to MapVolume and saved. DONE!')
else:
    print('[FloorPlan] WARNING: Could not load imported texture')
