"""Get exact world position/size of F4_SM_Cube60 (the blocker) and friends."""
import unreal

ews = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
gw = ews.get_game_world()

targets = ['F4_SM_Cube60', 'F4_SM_Cube59', 'F4_SM_Cube122']
sma = unreal.GameplayStatics.get_all_actors_of_class(gw, unreal.StaticMeshActor)

# Reference: player location at the stuck point
PLAYER = unreal.Vector(419.4, -1414.7, 1837.9)
STAIRS = unreal.Vector(749.0, -1467.9, 1501.1)

for label in targets:
    for a in sma:
        if a.get_actor_label() == label:
            loc = a.get_actor_location()
            rot = a.get_actor_rotation()
            scale = a.get_actor_scale3d()
            origin, extent = a.get_actor_bounds(only_colliding_components=True)
            smc = a.get_component_by_class(unreal.StaticMeshComponent)
            mesh = smc.static_mesh.get_name() if smc and smc.static_mesh else "?"
            print(f"\n=== {label} ===")
            print(f"  Mesh:  {mesh}")
            print(f"  Loc:   ({loc.x:.1f},{loc.y:.1f},{loc.z:.1f})")
            print(f"  Rot:   ({rot.pitch:.0f},{rot.yaw:.0f},{rot.roll:.0f})")
            print(f"  Scale: ({scale.x:.2f},{scale.y:.2f},{scale.z:.2f})")
            print(f"  Bounds origin ({origin.x:.0f},{origin.y:.0f},{origin.z:.0f}) extent ({extent.x:.0f},{extent.y:.0f},{extent.z:.0f})")
            print(f"  World box: x=[{origin.x-extent.x:.0f},{origin.x+extent.x:.0f}] y=[{origin.y-extent.y:.0f},{origin.y+extent.y:.0f}] z=[{origin.z-extent.z:.0f},{origin.z+extent.z:.0f}]")
            # Distance from player at stuck-spot
            dxy = ((loc.x - PLAYER.x)**2 + (loc.y - PLAYER.y)**2) ** 0.5
            print(f"  XY distance from stuck-spot player: {dxy:.1f} UU")
            # Local pos relative to F4_SM_Steps2
            inv = unreal.Transform(unreal.Rotator(0,0,0), STAIRS, unreal.Vector(1,1,1)).inverse()
            rel = inv.transform_location(loc)
            print(f"  Local in F4_SM_Steps2 frame: ({rel.x:.1f},{rel.y:.1f},{rel.z:.1f})")
            # Check if it's part of a Blueprint actor or a plain StaticMeshActor
            print(f"  Class: {a.get_class().get_name()}")
            print(f"  Component collision: enabled={smc.get_collision_enabled()} profile={smc.get_collision_profile_name()}")
            break
    else:
        print(f"{label}: NOT FOUND")

# Also: enumerate ALL F4_*Cube* actors near the stair turn — could be more
print("\n=== ALL F4_*Cube*/F4_*Column* actors within 600 UU of player stuck-spot ===")
for a in sma:
    lbl = a.get_actor_label()
    if not lbl.startswith("F4_"):
        continue
    if ("Cube" not in lbl) and ("Column" not in lbl) and ("Pillar" not in lbl) and ("Post" not in lbl):
        continue
    loc = a.get_actor_location()
    dxy = ((loc.x - PLAYER.x)**2 + (loc.y - PLAYER.y)**2 + (loc.z - PLAYER.z)**2) ** 0.5
    if dxy < 600:
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mesh = smc.static_mesh.get_name() if smc and smc.static_mesh else "?"
        origin, extent = a.get_actor_bounds(only_colliding_components=True)
        print(f"  {lbl:24s} [{mesh}] @ ({loc.x:.0f},{loc.y:.0f},{loc.z:.0f}) extent ({extent.x:.0f},{extent.y:.0f},{extent.z:.0f}) dist={dxy:.0f}")

print("\nDone.")
