"""Line trace along the elevator wall to find window/glass openings.
Traces from corridor side (Y > wall) toward wall (Y < wall) at multiple X positions."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = eas.get_world()

WALL_Y = -1316.0
TRACE_START_Y = -1280.0  # corridor side
TRACE_END_Y = -1360.0    # through the wall
TRACE_Z = 2150.0         # mid-height on F5

print("=== Line traces along elevator wall on F5 ===")
print(f"Z={TRACE_Z}, tracing from Y={TRACE_START_Y} to Y={TRACE_END_Y}\n")

for x in range(350, 1750, 25):
    start = unreal.Vector(x, TRACE_START_Y, TRACE_Z)
    end = unreal.Vector(x, TRACE_END_Y, TRACE_Z)

    hit_result = unreal.SystemLibrary.line_trace_single(
        world,
        start, end,
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True,   # trace complex
        [],     # actors to ignore
        unreal.DrawDebugTrace.NONE,
        True    # ignore self
    )
    hit = hit_result is not None
    result = hit_result if hit_result else unreal.HitResult()

    if hit:
        actor = result.get_editor_property('hit_actor')
        impact = result.get_editor_property('impact_point')
        if actor:
            label = actor.get_actor_label()
            print(f"  X={x:5d} | HIT {label:40s} | impact_y={impact.y:.1f}")
    else:
        print(f"  X={x:5d} | NO HIT (window opening?)")
