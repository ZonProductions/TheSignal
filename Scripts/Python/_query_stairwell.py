"""Query all actors in the stairwell area to find floor layers."""
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = eas.get_all_level_actors()

# Stairwell is near the Exit_door at X~939, Y~-1324
# Let's search a wider area around the stairwell
STAIR_X_MIN = 800
STAIR_X_MAX = 1100
STAIR_Y_MIN = -1500
STAIR_Y_MAX = -1150

stair_actors = []
for a in all_actors:
    loc = a.get_actor_location()
    if (STAIR_X_MIN <= loc.x <= STAIR_X_MAX and
        STAIR_Y_MIN <= loc.y <= STAIR_Y_MAX):
        stair_actors.append(a)

print(f"=== Stairwell area actors: {len(stair_actors)} ===")
print(f"Area: X({STAIR_X_MIN}-{STAIR_X_MAX}) Y({STAIR_Y_MIN}-{STAIR_Y_MAX})")

# Sort by Z to see the floor layers
stair_actors.sort(key=lambda a: a.get_actor_location().z)

for a in stair_actors:
    loc = a.get_actor_location()
    label = a.get_actor_label()
    cls = a.get_class().get_name()
    # Get mesh name if StaticMeshActor
    mesh_name = ""
    if cls == "StaticMeshActor":
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            sm = smc.get_editor_property("static_mesh")
            if sm:
                mesh_name = sm.get_name()
    print(f"  Z={loc.z:7.0f} | {cls:25s} | {label:45s} | {mesh_name}")
