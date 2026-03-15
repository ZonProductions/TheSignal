
import unreal
import math

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

FLOOR_Z_MIN = 1900
FLOOR_Z_MAX = 2300
FLOOR_LEVEL_Z = 1987

wall_meshes = {"SM_Cube","SM_Column","SM_Cylinder","SM_WcWall","SM_Securitywall","SM_FrameTall","SM_FrameTallDoor","SM_LastLineEndWall","SM_CenterRoomsinnerWall","SM_ElevatorWall","SM_Elevator","SM_Room2SideGlass","SM_SecuritySilling","SM_SillingTile","SM_SillingCompoDark","SM_ThinBoxHorizen","SM_ThinBoxVertical","SM_ThinBoxHorizenDouble","SM_ThinBoxVerticalDouble","SM_WebPartitionFrame","SM_PartitionWorkSpace","SM_WorkStation_Partition","SM_Fence","SM_Steps","SM_Antena","SM_DoorOfficeFrame","SM_DoorExitFrame","SM_AutoDoorBase"}
room_meshes = {"SM_RoomManagerA","SM_RoomManagerB","SM_ConferenceSecretaryRoom"}
floor_meshes = {"SM_Woodfloor","SM_OutsideFloor","SM_KitchenFloor"}
door_meshes = {"SM_DoorOffice","SM_DoorExit","SM_AutoDoorLeft","SM_AutoDoorRight","SM_RoomManagerDoor"}
door_bp_classes = {"BP_WCDoor01_C","BP_WCDoor02_C","BP_GlassDoors1_C","BP_ElevatorDoors_C","ZP_InteractDoor"}
window_meshes = {"SM_WindowWall"}
ladder_classes = {"ZP_Ladder","BP_Ladder_C"}

def get_rect(actor, comp):
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    rot = actor.get_actor_rotation()
    bmin, bmax = comp.get_local_bounds()
    sx, sy, sz = abs(scale.x), abs(scale.y), abs(scale.z)
    # When roll ~= +/-90, Y and Z axes swap (BigCompany wallBrick pattern)
    # Roll rotates around X axis: Y becomes height, Z becomes horizontal Y
    if abs(abs(rot.roll) - 90) < 15:
        sy, sz = sz, sy
    half_x = (bmax.x - bmin.x) * sx / 2.0
    half_y = (bmax.y - bmin.y) * sy / 2.0
    cx = loc.x + (bmin.x + bmax.x) / 2.0 * scale.x
    cy = loc.y + (bmin.y + bmax.y) / 2.0 * scale.y
    yaw_rad = math.radians(rot.yaw)
    cos_y = math.cos(yaw_rad)
    sin_y = math.sin(yaw_rad)
    corners = [(-half_x,-half_y),(half_x,-half_y),(half_x,half_y),(-half_x,half_y)]
    return [(cx+lx*cos_y-ly*sin_y, cy+lx*sin_y+ly*cos_y) for lx,ly in corners]

walls = []
doors = []
windows = []
floors = []
ladders = []
rooms = []

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
print(f"DONE: {len(walls)} walls, {len(floors)} floors, {len(rooms)} rooms, {len(windows)} windows, {len(ladders)} ladders")
