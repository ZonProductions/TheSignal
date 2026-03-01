import unreal
import time

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)
a = actors[0]
mesh = a.get_editor_property("Mesh")

# Get actor location to compute relative bone position
actor_loc = a.get_actor_location()

# Sample 1
foot1 = mesh.get_socket_location("foot_l")
rel1_z = foot1.z - actor_loc.z
print(f"Sample1 - actor:({actor_loc.x:.0f},{actor_loc.y:.0f},{actor_loc.z:.0f}) foot_l:({foot1.x:.1f},{foot1.y:.1f},{foot1.z:.1f}) rel_z:{rel1_z:.2f}")

# Wait
time.sleep(0.5)

# Sample 2
actor_loc2 = a.get_actor_location()
foot2 = mesh.get_socket_location("foot_l")
rel2_z = foot2.z - actor_loc2.z
print(f"Sample2 - actor:({actor_loc2.x:.0f},{actor_loc2.y:.0f},{actor_loc2.z:.0f}) foot_l:({foot2.x:.1f},{foot2.y:.1f},{foot2.z:.1f}) rel_z:{rel2_z:.2f}")

print(f"Foot relative Z delta: {abs(rel2_z - rel1_z):.4f}")
if abs(rel2_z - rel1_z) < 0.01:
    print("RESULT: Hidden mesh NOT animating — foot position static relative to actor")
else:
    print("RESULT: Hidden mesh IS animating — foot position changing")

# Same for PlayerMesh
pm = a.get_editor_property("PlayerMesh")
pfoot1 = pm.get_socket_location("foot_l")
prel1 = pfoot1.z - actor_loc.z
time.sleep(0.5)
actor_loc3 = a.get_actor_location()
pfoot2 = pm.get_socket_location("foot_l")
prel2 = pfoot2.z - actor_loc3.z
print(f"PlayerMesh foot_l rel_z: {prel1:.2f} -> {prel2:.2f}, delta: {abs(prel2-prel1):.4f}")
