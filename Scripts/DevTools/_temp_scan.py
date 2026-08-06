
import unreal
import math

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

FLOOR_Z_MIN = 1900
FLOOR_Z_MAX = 2400
FLOOR_LEVEL_Z = 2000

wall_meshes = {"SM_Cube","SM_Column","SM_Cylinder","SM_WcWall","SM_Securitywall","SM_FrameTall","SM_FrameTallDoor","SM_LastLineEndWall","SM_CenterRoomsinnerWall","SM_ElevatorWall","SM_Elevator","SM_Room2SideGlass","SM_SecuritySilling","SM_SillingTile","SM_SillingCompoDark","SM_ThinBoxHorizen","SM_ThinBoxVertical","SM_ThinBoxHorizenDouble","SM_ThinBoxVerticalDouble","SM_WebPartitionFrame","SM_PartitionWorkSpace","SM_WorkStation_Partition","SM_Fence","SM_Steps","SM_Antena","SM_DoorOfficeFrame","SM_DoorExitFrame","SM_AutoDoorBase","SM_wallBrick",
    # ResearchFacility pack: structural verticals + railings read as plan lines.
    # (SM_Wall_* is already covered by the "wall" keyword below.)
    "SM_Colomn_1","SM_Colomn_2","SM_Railing_1","SM_Elevator_Wall_1","SM_Elevator_Wall_4","SM_Elevator_Wall_5"}
# Any mesh whose name contains one of these (case-insensitive) is treated as a wall: covers
# SM_wallBrick + other maps' wall meshes without listing every one. Add map-specific keywords here.
wall_keywords = ("wall", "brick")
room_meshes = {"SM_RoomManagerA","SM_RoomManagerB","SM_ConferenceSecretaryRoom"}
# ResearchFacility slabs: SM_Floor_4 (main deck, x95) / SM_Floor_1 (lower + sub-basement).
floor_meshes = {"SM_Woodfloor","SM_OutsideFloor","SM_KitchenFloor","SM_Floor_1","SM_Floor_4","SM_Elevator_Floor_1","SM_Elevator_Floor_2"}
door_meshes = {"SM_DoorOffice","SM_DoorExit","SM_AutoDoorLeft","SM_AutoDoorRight","SM_RoomManagerDoor"}
door_bp_classes = {"BP_WCDoor01_C","BP_WCDoor02_C","BP_GlassDoors1_C","BP_ElevatorDoors_C","ZP_InteractDoor"}
window_meshes = {"SM_WindowWall"}
ladder_classes = {"ZP_Ladder","BP_Ladder_C"}

# Monotone chain. Input is <=8 projected corners, so this is trivially cheap.
def _convex_hull(pts):
    pts = sorted(set((round(x, 3), round(y, 3)) for x, y in pts))
    if len(pts) <= 2:
        return pts

    def cross(o, a, b):
        return (a[0]-o[0])*(b[1]-o[1]) - (a[1]-o[1])*(b[0]-o[0])

    lower = []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    upper = []
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


# Smallest-area enclosing rectangle (rotating calipers over hull edges). The minimum-area
# rect always has a side flush with a hull edge, so testing every edge orientation is exact.
# Returns 4 corners in order, or None if degenerate.
def _min_area_rect(hull):
    n = len(hull)
    if n < 3:
        return None
    best = None
    for i in range(n):
        x1, y1 = hull[i]
        x2, y2 = hull[(i + 1) % n]
        ex, ey = x2 - x1, y2 - y1
        L = math.hypot(ex, ey)
        if L < 1e-6:
            continue
        ux, uy = ex / L, ey / L      # edge direction
        vx, vy = -uy, ux             # its normal
        us = [p[0]*ux + p[1]*uy for p in hull]
        vs = [p[0]*vx + p[1]*vy for p in hull]
        umin, umax, vmin, vmax = min(us), max(us), min(vs), max(vs)
        area = (umax - umin) * (vmax - vmin)
        if best is None or area < best[0]:
            best = (area, ux, uy, vx, vy, umin, umax, vmin, vmax)
    if best is None:
        return None
    _, ux, uy, vx, vy, umin, umax, vmin, vmax = best
    return [(ux*u + vx*v, uy*u + vy*v)
            for u, v in ((umin, vmin), (umax, vmin), (umax, vmax), (umin, vmax))]


# World-space XY footprint of a placed mesh.
#
# Pushes all 8 corners of the component's LOCAL bounding box through its full world
# transform, then takes the minimum-area rectangle of the projected silhouette.
#
# This replaces hand-rolled yaw trig that was WRONG for any mesh whose pivot is not
# centred on its bounds. The old math added the pivot->centre offset along WORLD X/Y
# while the offset is a LOCAL-space vector, so it had to be rotated first. Every wall
# mesh in ResearchFacility pivots at an end or corner (SM_Wall_8 local X is 0..280,
# SM_Wall_12 is 0..480 in both X and Y), which displaced 722 of 973 walls by up to
# ~680 UU - walls landed metres off their true positions and no run lined up.
# Measured 2026-08-06: only the yaw==0 walls were ever correct.
#
# Using the component's own world transform also fixes, for free:
#   - pitch (the old code ignored it entirely)
#   - roll (the old code special-cased +/-90 with a sy/sz swap and nothing else)
#   - negative / non-uniform scale
#   - components with a relative transform (the old code read the ACTOR transform,
#     so any mesh offset inside its Blueprint was placed at the actor's origin)
def _world_corners(comp):
    bmin, bmax = comp.get_local_bounds()
    xf = comp.get_world_transform()
    pts = []
    for ix in (bmin.x, bmax.x):
        for iy in (bmin.y, bmax.y):
            for iz in (bmin.z, bmax.z):
                w = xf.transform_location(unreal.Vector(ix, iy, iz))
                pts.append((w.x, w.y, w.z))
    return pts


def get_rect(actor, comp):
    pts = [(p[0], p[1]) for p in _world_corners(comp)]
    rect = _min_area_rect(_convex_hull(pts))
    if rect is None:
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        rect = [(min(xs), min(ys)), (max(xs), min(ys)),
                (max(xs), max(ys)), (min(xs), max(ys))]
    return rect


# ---------------------------------------------------------------- surface dividers
#
# BP_Surface is the floor/ceiling DIVIDER: it is the deck the player walks on, and
# nothing beneath it belongs on this floor's plan. A Z band alone cannot express that
# - geometry tucked just under the deck still has its pivot inside the band, so it got
# drawn straight over the rooms as a phantom "platform".
#
# ResearchFacility has two, stacked:
#   BP_Surface   Z 3020..3030   <- floor of the main deck
#   BP_Surface2  Z 3370.8       <- its ceiling / floor of the deck above
#
# So each floor is the slab BETWEEN two dividers. We pick the divider nearest this
# band's FLOOR_LEVEL_Z as the floor, the next one up as the ceiling, and drop anything
# lying wholly outside that sandwich. Levels with no BP_Surface (Building1) are
# unaffected - SURFACES stays empty and nothing is culled.
surface_class_keywords = ("Surface",)
SURFACE_MATCH_RANGE = 200.0   # how near a divider must be to FLOOR_LEVEL_Z to be "this floor's"
SURFACE_EPS = 1.0

SURFACES = []
for _a in eas.get_all_level_actors():
    if not any(k in _a.get_class().get_name() for k in surface_class_keywords):
        continue
    _o, _e = _a.get_actor_bounds(False)
    SURFACES.append({
        "label": _a.get_actor_label(),
        "top": _o.z + _e.z,
        "x0": _o.x - _e.x, "x1": _o.x + _e.x,
        "y0": _o.y - _e.y, "y1": _o.y + _e.y,
    })
SURFACES.sort(key=lambda s: s["top"])

FLOOR_DIVIDER = None
CEILING_DIVIDER = None
for _s in SURFACES:
    if abs(_s["top"] - FLOOR_LEVEL_Z) <= SURFACE_MATCH_RANGE:
        if FLOOR_DIVIDER is None or _s["top"] > FLOOR_DIVIDER["top"]:
            FLOOR_DIVIDER = _s
if FLOOR_DIVIDER is not None:
    for _s in SURFACES:
        if _s["top"] > FLOOR_DIVIDER["top"] + 50.0:
            CEILING_DIVIDER = _s
            break

for _s in SURFACES:
    _role = "floor" if _s is FLOOR_DIVIDER else ("ceiling" if _s is CEILING_DIVIDER else "-")
    print("SURFACE: %s top=%.1f X %.0f..%.0f Y %.0f..%.0f  role=%s"
          % (_s["label"], _s["top"], _s["x0"], _s["x1"], _s["y0"], _s["y1"], _role))


def _covers(surface, xs, ys):
    # True if the actor's XY footprint overlaps the divider's footprint, i.e. the
    # divider is actually over/under it. Geometry off the edge of the deck is left alone.
    return not (max(xs) < surface["x0"] or min(xs) > surface["x1"] or
                max(ys) < surface["y0"] or min(ys) > surface["y1"])


def hidden_by_surface(pts):
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    zs = [p[2] for p in pts]
    if FLOOR_DIVIDER is not None and max(zs) <= FLOOR_DIVIDER["top"] + SURFACE_EPS \
            and _covers(FLOOR_DIVIDER, xs, ys):
        return True
    if CEILING_DIVIDER is not None and min(zs) >= CEILING_DIVIDER["top"] - SURFACE_EPS \
            and _covers(CEILING_DIVIDER, xs, ys):
        return True
    return False

walls = []
doors = []
windows = []
floors = []
ladders = []
rooms = []
culled = 0

for a in eas.get_all_level_actors():
    loc = a.get_actor_location()
    if loc.z < FLOOR_Z_MIN or loc.z > FLOOR_Z_MAX: continue
    cn = a.get_class().get_name()
    if cn == "StaticMeshActor":
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps: continue
        sm = comps[0].get_editor_property("static_mesh")
        if not sm: continue
        mn = sm.get_name()
        # BP_Surface is the floor/ceiling divider - anything wholly beneath the deck
        # (or above its ceiling) is not part of THIS floor's plan.
        if hidden_by_surface(_world_corners(comps[0])):
            culled += 1
            continue
        if mn == "SM_Cube":
            s = a.get_actor_scale3d()
            if abs(s.x-1.0)<0.1 and abs(s.y-1.0)<0.1 and s.z<0.5 and abs(loc.z-FLOOR_LEVEL_Z)<20: continue
            if abs(s.x)<0.05 and abs(s.y)<0.05: continue
            if s.z<0.3 and loc.z > FLOOR_LEVEL_Z + 150: continue
            r = a.get_actor_rotation()
            if abs(r.pitch) > 10 and abs(r.roll) < 10: continue
            walls.append(get_rect(a, comps[0]))
        elif mn in window_meshes: windows.append(get_rect(a, comps[0]))
        elif mn in floor_meshes: floors.append(get_rect(a, comps[0]))
        elif mn in room_meshes: rooms.append(get_rect(a, comps[0]))
        elif mn in wall_meshes: walls.append(get_rect(a, comps[0]))
        elif any(k in mn.lower() for k in wall_keywords): walls.append(get_rect(a, comps[0]))
    elif cn in ladder_classes:
        x,y = loc.x, loc.y
        ladders.append([(x-30,y-15),(x+30,y-15),(x+30,y+15),(x-30,y+15)])

print("WALLS")
for r in walls:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("FLOORS")
for r in floors:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("ROOMS")
for r in rooms:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("DOORS")
print("WINDOWS")
for r in windows:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print("LADDERS")
for r in ladders:
    print(f"R:{r[0][0]:.1f},{r[0][1]:.1f},{r[1][0]:.1f},{r[1][1]:.1f},{r[2][0]:.1f},{r[2][1]:.1f},{r[3][0]:.1f},{r[3][1]:.1f}")
print(f"DONE: {len(walls)} walls, {len(floors)} floors, {len(rooms)} rooms, {len(windows)} windows, {len(ladders)} ladders, {culled} hidden by surface")
