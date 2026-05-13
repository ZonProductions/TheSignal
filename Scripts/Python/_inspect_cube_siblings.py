"""Inspect F4_SM_Cube59, F4_SM_Cube60, F4_SM_Cube122 to understand layout.

Goal: figure out if they're parallel slabs forming a wall, or independent
obstructions. Also dump what's BEHIND F4_SM_Cube60 so we know if disabling
its collision would expose a hole.
"""
import unreal

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world() or ews.get_editor_world()
print(f"Using world: {gw}")

targets = ['F4_SM_Cube59', 'F4_SM_Cube60', 'F4_SM_Cube122']
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)

actors = {}
for a in sma:
    lbl = a.get_actor_label()
    if lbl in targets:
        actors[lbl] = a

for lbl, a in actors.items():
    loc = a.get_actor_location()
    rot = a.get_actor_rotation()
    scale = a.get_actor_scale3d()
    origin, extent = a.get_actor_bounds(only_colliding_components=True)
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    print(f"\n=== {lbl} ===")
    print(f"  Loc:   ({loc.x:.1f},{loc.y:.1f},{loc.z:.1f})")
    print(f"  Rot:   ({rot.pitch:.0f},{rot.yaw:.0f},{rot.roll:.0f})  Scale: ({scale.x:.2f},{scale.y:.2f},{scale.z:.2f})")
    print(f"  World box: x=[{origin.x-extent.x:.0f},{origin.x+extent.x:.0f}] y=[{origin.y-extent.y:.0f},{origin.y+extent.y:.0f}] z=[{origin.z-extent.z:.0f},{origin.z+extent.z:.0f}]")
    print(f"  Component collision: enabled={smc.get_collision_enabled()} profile={smc.get_collision_profile_name()}")

# Now: what's BEHIND F4_SM_Cube60 in the -X direction?
# Slab is at X=[346, 364]. Trace from inside the slab going -X to find the next blocker.
cube60 = actors.get('F4_SM_Cube60')
if cube60:
    loc = cube60.get_actor_location()
    # Start trace just behind the slab (X-side), pointing -X
    start = unreal.Vector(loc.x - 20, loc.y, loc.z)  # 20 UU behind slab
    end   = unreal.Vector(loc.x - 500, loc.y, loc.z) # 500 UU behind
    hit = unreal.SystemLibrary.line_trace_single(
        gw, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        False, [cube60], unreal.DrawDebugTrace.NONE, True)
    if hit:
        # Use the right attribute name for HitResult in 5.4
        try:
            hloc = hit.location
        except Exception:
            hloc = hit.get_editor_property('location')
        try:
            hactor = hit.hit_actor
        except Exception:
            hactor = hit.get_editor_property('hit_actor')
        try:
            hcomp = hit.component
        except Exception:
            hcomp = hit.get_editor_property('component')
        dist = abs(hloc.x - start.x)
        print(f"\n=== What is BEHIND F4_SM_Cube60 in -X direction? ===")
        print(f"  Hit at ({hloc.x:.1f},{hloc.y:.1f},{hloc.z:.1f}) dist behind slab: {dist:.1f} UU")
        if hactor:
            print(f"  Actor: {hactor.get_actor_label()} [{type(hactor).__name__}]")
        if hcomp:
            sm_name = "?"
            if hasattr(hcomp, 'static_mesh') and hcomp.static_mesh:
                sm_name = hcomp.static_mesh.get_name()
            print(f"  Comp: {hcomp.get_name()}  Mesh: {sm_name}")
    else:
        print(f"\nNothing behind F4_SM_Cube60 in -X direction within 500 UU — disabling its collision would expose open space")

print("\nDone.")
