import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)[0]
mesh = a.get_editor_property("Mesh")
pm = a.get_editor_property("PlayerMesh")

# Store frame 1 data using tick-based approach
# (can't use sleep - blocks game thread)
# Just print current values - will run this twice via separate curl calls
loc = a.get_actor_location()
hfoot = mesh.get_socket_location("foot_l")
pfoot = pm.get_socket_location("foot_l")
hfoot_rel_z = hfoot.z - loc.z
pfoot_rel_z = pfoot.z - loc.z
print(f"HiddenMesh foot_l rel_z: {hfoot_rel_z:.4f}")
print(f"PlayerMesh foot_l rel_z: {pfoot_rel_z:.4f}")

# Also check thigh for rotation
hthigh = mesh.get_socket_location("thigh_l")
pthigh = pm.get_socket_location("thigh_l")
print(f"HiddenMesh thigh_l rel: ({hthigh.x-loc.x:.2f}, {hthigh.y-loc.y:.2f}, {hthigh.z-loc.z:.2f})")
print(f"PlayerMesh thigh_l rel: ({pthigh.x-loc.x:.2f}, {pthigh.y-loc.y:.2f}, {pthigh.z-loc.z:.2f})")
