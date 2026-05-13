"""Inspect both stairwells' walls + neighboring actors at the Y-seams.

For each F4_SM_Steps* actor, identify the analog of F4_SM_Cube60 (the outer
landing wall) and what abuts it at the seam Y positions. That tells us if
we can safely push the wall back without exposing a gap.
"""
import unreal

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
print(f"World: {gw}")

sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)
print(f"StaticMeshActors in world: {len(sma)}")

# Locate the two stairwells' walls
# F4_SM_Steps2 stairwell: outer wall is F4_SM_Cube60 (known)
# F4_SM_Steps3 stairwell: need to find equivalent
# F4_SM_Steps3 is at world (-3192, 5263, 1501) rot=(0,180,0)
# By 180° symmetry, expect a Cube wall near the SM_Steps3 origin, oriented similarly

steps2 = None
steps3 = None
cube60 = None
for a in sma:
    lbl = a.get_actor_label()
    if lbl == "F4_SM_Steps2": steps2 = a
    elif lbl == "F4_SM_Steps3": steps3 = a
    elif lbl == "F4_SM_Cube60": cube60 = a

if not steps3:
    print("ERROR: F4_SM_Steps3 not found"); raise SystemExit
if not cube60:
    print("ERROR: F4_SM_Cube60 not found"); raise SystemExit

s3_loc = steps3.get_actor_location()
print(f"\nF4_SM_Steps3 @ ({s3_loc.x:.1f},{s3_loc.y:.1f},{s3_loc.z:.1f})")

# Find F4_SM_Cube* actors near F4_SM_Steps3 that look like a wall slab (thin, tall)
print(f"\nLooking for outer-wall slab matching Cube60 near F4_SM_Steps3 (within 600 UU):")
cube60_loc = cube60.get_actor_location()
# Relative offset from steps2 to cube60 (in world space, both with rot=0):
# steps2 @ (749, -1468, 1501), cube60 @ (355, -1473, 1700)
# offset = cube60 - steps2 = (-394, -5, 199)
# For steps3 (rotated 180° around Z), the equivalent wall should be at:
# steps3_loc + (-offset.x, -offset.y, offset.z) = -3192 + 394, 5263 + 5, 1501 + 199
# = (-2798, 5268, 1700)
expected_x = -3192 + 394  # = -2798
expected_y = 5263 + 5     # = 5268
expected_z = 1501 + 199   # = 1700
print(f"  Expected mirrored wall location ~({expected_x},{expected_y},{expected_z})")

candidates = []
for a in sma:
    lbl = a.get_actor_label()
    if not lbl.startswith("F4_") or "Cube" not in lbl:
        continue
    loc = a.get_actor_location()
    if abs(loc.x - expected_x) < 100 and abs(loc.y - expected_y) < 200 and abs(loc.z - expected_z) < 50:
        scale = a.get_actor_scale3d()
        origin, extent = a.get_actor_bounds(only_colliding_components=True)
        candidates.append((lbl, loc, scale, extent, a))
        print(f"  CANDIDATE: {lbl} @ ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) scale=({scale.x:.2f},{scale.y:.2f},{scale.z:.2f}) extent=({extent.x:.0f},{extent.y:.0f},{extent.z:.0f})")

# Pick the wall slab (large Y extent, thin X)
wall_for_steps3 = None
for lbl, loc, scale, extent, a in candidates:
    if extent.y > 100 and extent.x < 20:
        wall_for_steps3 = a
        print(f"  --> PICKED as F4_SM_Steps3 outer wall: {lbl}")
        break

# Now inspect what abuts Cube60 at its Y seams
print(f"\n=== Neighbors of F4_SM_Cube60 at its Y-seams ===")
print(f"Cube60 world box: x=[346,364] y=[-1619,-1327] z=[1426,1976]")

# Probe just past each Y end of the wall — and at varying Z heights
def probe_seam(label, x, y, z, radius=30):
    overlaps = unreal.SystemLibrary.box_overlap_actors(
        gw, unreal.Vector(x, y, z),
        unreal.Vector(radius, radius, radius),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1,
         unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2],
        None, [cube60])
    names = []
    for h in (overlaps or []):
        if h is cube60: continue
        smc = h.get_component_by_class(unreal.StaticMeshComponent) if hasattr(h, 'get_component_by_class') else None
        mesh = smc.static_mesh.get_name() if (smc and smc.static_mesh) else "?"
        bo, be = h.get_actor_bounds(only_colliding_components=True)
        names.append(f"{h.get_actor_label()}[{mesh}]@({bo.x:.0f},{bo.y:.0f},{bo.z:.0f})±({be.x:.0f},{be.y:.0f},{be.z:.0f})")
    print(f"  {label} probe @ ({x},{y},{z}) ±{radius}: {len(names)} actors")
    for n in names[:8]:
        print(f"    {n}")
    if len(names) > 8:
        print(f"    +{len(names)-8} more")

# Probe at -Y seam (Y=-1619) and +Y seam (Y=-1327), at mid-height
probe_seam("-Y seam mid",  355, -1640, 1701)
probe_seam("+Y seam mid",  355, -1310, 1701)
probe_seam("-Y seam low",  355, -1640, 1450)
probe_seam("+Y seam low",  355, -1310, 1450)
probe_seam("-X behind mid", 320, -1473, 1701)  # 35 UU behind the wall
probe_seam("-X way behind", 280, -1473, 1701)  # 75 UU behind the wall

print("\nDone.")
