import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# A representative window wall: F3_SM_WindowWall9 loc=(1147,1224,972) yaw=0, thin in Y (perimeter runs along X)
# Find StaticMeshActors NEAR that perimeter line (y~1224) on floor 3 that are NOT window walls,
# and are wall-shaped (thin in one axis, tall). Report mesh + scale + bounds footprint.
import math
def comp(a):
    cs = a.get_components_by_class(unreal.StaticMeshComponent)
    return cs[0] if cs else None

print("=== solid wall candidates near perimeter y~1224, floor3 z~972 ===")
tally = {}
for a in eas.get_all_level_actors():
    c = comp(a)
    if not c: continue
    sm = c.get_editor_property("static_mesh")
    if not sm: continue
    mn = sm.get_name()
    if mn == "SM_WindowWall": continue
    loc = a.get_actor_location()
    if abs(loc.y - 1224) > 120: continue
    if not (800 <= loc.z <= 1150): continue
    # world bounds extent
    org, ext = a.get_actor_bounds(False)
    # wall-shaped: tall and one horizontal axis thin
    if ext.z < 150: continue
    tally[mn] = tally.get(mn, 0) + 1
    if tally[mn] <= 2:
        s = a.get_actor_scale3d()
        print("  %-26s mesh=%-20s loc=(%.0f,%.0f,%.0f) ext=(%.0f,%.0f,%.0f) scale=(%.2f,%.2f,%.2f)" % (
            a.get_actor_label(), mn, loc.x, loc.y, loc.z, ext.x, ext.y, ext.z, s.x, s.y, s.z))
print("perimeter wall mesh tally:", tally)

# Also: dimensions of common candidate meshes
for path in ["/Game/office_BigCompanyArchViz/StaticMesh/Environment/SM_Cube.SM_Cube",
             "/Engine/BasicShapes/Cube.Cube"]:
    m = unreal.load_asset(path)
    if m:
        e = m.get_bounds().box_extent
        print("MESH %s extent=(%.1f,%.1f,%.1f)" % (m.get_name(), e.x, e.y, e.z))
