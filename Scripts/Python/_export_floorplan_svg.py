import unreal
import os
import math

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Config
FLOOR_Z_MIN = 1900
FLOOR_Z_MAX = 2300
FLOOR_LEVEL_Z = 1987
SCALE = 0.1  # UU to SVG pixels (100 UU = 10px = 1 meter)
PADDING = 50  # SVG padding in pixels
OUTPUT_PATH = "C:/Users/Ommei/workspace/TheSignal/Scripts/FloorPlans/F5.svg"

wall_meshes = {
    "SM_Cube", "SM_Column", "SM_Cylinder",
    "SM_WindowWall", "SM_WcWall", "SM_Securitywall",
    "SM_FrameTall", "SM_FrameTallDoor", "SM_LastLineEndWall",
    "SM_CenterRoomsinnerWall", "SM_ElevatorWall",
    "SM_Elevator",
}

door_meshes = {
    "SM_DoorOffice", "SM_DoorOfficeFrame",
    "SM_DoorExit", "SM_DoorExitFrame",
    "SM_AutoDoorBase", "SM_AutoDoorLeft", "SM_AutoDoorRight",
    "SM_RoomManagerDoor",
}

door_bp_classes = {"BP_WCDoor01_C", "BP_WCDoor02_C", "BP_GlassDoors1_C", "BP_ElevatorDoors_C", "ZP_InteractDoor"}

window_meshes = {"SM_WindowWall"}

def get_rect(actor, comp):
    """Get world-space XY rectangle corners accounting for rotation."""
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    rot = actor.get_actor_rotation()
    bmin, bmax = comp.get_local_bounds()

    # Local corners (XY only)
    half_x = (bmax.x - bmin.x) * abs(scale.x) / 2.0
    half_y = (bmax.y - bmin.y) * abs(scale.y) / 2.0
    cx = loc.x + (bmin.x + bmax.x) / 2.0 * scale.x
    cy = loc.y + (bmin.y + bmax.y) / 2.0 * scale.y

    # Rotate corners by yaw
    yaw_rad = math.radians(rot.yaw)
    cos_y = math.cos(yaw_rad)
    sin_y = math.sin(yaw_rad)

    corners = [
        (-half_x, -half_y),
        ( half_x, -half_y),
        ( half_x,  half_y),
        (-half_x,  half_y),
    ]

    world_corners = []
    for lx, ly in corners:
        wx = cx + lx * cos_y - ly * sin_y
        wy = cy + lx * sin_y + ly * cos_y
        world_corners.append((wx, wy))

    return world_corners

# Collect geometry
walls = []  # list of (corners, mesh_name)
doors = []
windows = []

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
            # Skip floor tiles
            is_floor = (abs(s.x - 1.0) < 0.1 and abs(s.y - 1.0) < 0.1 and s.z < 0.5 and abs(loc.z - FLOOR_LEVEL_Z) < 20)
            if is_floor:
                continue
            corners = get_rect(a, comps[0])
            walls.append((corners, mn))
        elif mn in window_meshes:
            corners = get_rect(a, comps[0])
            windows.append((corners, mn))
        elif mn in wall_meshes:
            corners = get_rect(a, comps[0])
            walls.append((corners, mn))
        elif mn in door_meshes:
            corners = get_rect(a, comps[0])
            doors.append((corners, mn))

    elif cn in door_bp_classes:
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if comps:
            corners = get_rect(a, comps[0])
            doors.append((corners, cn))
        else:
            # Fallback: small square at location
            x, y = loc.x, loc.y
            doors.append(([(x-25,y-25),(x+25,y-25),(x+25,y+25),(x-25,y+25)], cn))

print(f"F5: {len(walls)} walls, {len(doors)} doors, {len(windows)} windows")

# Find global extent
all_corners = []
for corners, _ in walls + doors + windows:
    all_corners.extend(corners)

min_x = min(c[0] for c in all_corners)
max_x = max(c[0] for c in all_corners)
min_y = min(c[1] for c in all_corners)
max_y = max(c[1] for c in all_corners)

svg_w = (max_x - min_x) * SCALE + PADDING * 2
svg_h = (max_y - min_y) * SCALE + PADDING * 2

def to_svg(wx, wy):
    """World coords to SVG coords."""
    sx = (wx - min_x) * SCALE + PADDING
    sy = (wy - min_y) * SCALE + PADDING
    return (sx, sy)

def polygon_points(corners):
    return " ".join(f"{to_svg(x,y)[0]:.1f},{to_svg(x,y)[1]:.1f}" for x, y in corners)

# Build SVG
lines = []
lines.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{svg_w:.0f}" height="{svg_h:.0f}" viewBox="0 0 {svg_w:.0f} {svg_h:.0f}">')
lines.append(f'<rect width="100%" height="100%" fill="#1a1a1a"/>')

# Grid lines (every 10 meters / 1000 UU)
lines.append('<g stroke="#2a2a2a" stroke-width="0.5">')
for gx in range(int(min_x / 1000) * 1000, int(max_x) + 1000, 1000):
    sx, _ = to_svg(gx, 0)
    lines.append(f'<line x1="{sx:.1f}" y1="0" x2="{sx:.1f}" y2="{svg_h:.0f}"/>')
for gy in range(int(min_y / 1000) * 1000, int(max_y) + 1000, 1000):
    _, sy = to_svg(0, gy)
    lines.append(f'<line x1="0" y1="{sy:.1f}" x2="{svg_w:.0f}" y2="{sy:.1f}"/>')
lines.append('</g>')

# Walls (dark gray fill, light outline)
lines.append('<g fill="#555555" stroke="#888888" stroke-width="0.5">')
for corners, mn in walls:
    pts = polygon_points(corners)
    lines.append(f'<polygon points="{pts}"/>')
lines.append('</g>')

# Windows (blue, thinner)
lines.append('<g fill="#334466" stroke="#5588cc" stroke-width="0.5">')
for corners, mn in windows:
    pts = polygon_points(corners)
    lines.append(f'<polygon points="{pts}"/>')
lines.append('</g>')

# Doors (orange)
lines.append('<g fill="#cc6622" stroke="#ff8833" stroke-width="0.5">')
for corners, mn in doors:
    pts = polygon_points(corners)
    lines.append(f'<polygon points="{pts}"/>')
lines.append('</g>')

# Scale bar (10 meters)
bar_x = PADDING
bar_y = svg_h - PADDING / 2
bar_len = 1000 * SCALE  # 10 meters
lines.append(f'<line x1="{bar_x}" y1="{bar_y}" x2="{bar_x + bar_len}" y2="{bar_y}" stroke="white" stroke-width="2"/>')
lines.append(f'<text x="{bar_x + bar_len / 2}" y="{bar_y - 5}" fill="white" font-size="10" text-anchor="middle">10m</text>')

lines.append('</svg>')

os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
with open(OUTPUT_PATH, 'w') as f:
    f.write('\n'.join(lines))

print(f"SVG saved: {OUTPUT_PATH}")
print(f"Canvas: {svg_w:.0f} x {svg_h:.0f} px")
