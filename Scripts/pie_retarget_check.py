import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)[0]
mesh = a.get_editor_property("Mesh")
pm = a.get_editor_property("PlayerMesh")
actor_loc = a.get_actor_location()

# Hidden mesh foot (should be moving since we forced walk)
hfoot = mesh.get_socket_location("foot_l")
print(f"Hidden foot_l rel: ({hfoot.x-actor_loc.x:.1f}, {hfoot.y-actor_loc.y:.1f}, {hfoot.z-actor_loc.z:.1f})")

# Player mesh foot (should be retargeted from hidden)
pfoot = pm.get_socket_location("foot_l")
print(f"Player foot_l rel: ({pfoot.x-actor_loc.x:.1f}, {pfoot.y-actor_loc.y:.1f}, {pfoot.z-actor_loc.z:.1f})")

# Check PlayerMesh anim instance
pai = pm.get_anim_instance()
print(f"PlayerMesh AnimInst: {pai.get_class().get_name()}")

# Check the SourceMeshComponent at runtime
src = pai.get_editor_property("SourceMeshComponent")
print(f"SourceMeshComponent: {src.get_name() if src else 'NONE'}")
if src:
    print(f"  Same as hidden mesh? {src == mesh}")
    src_ai = src.get_anim_instance()
    print(f"  Source AnimInst: {src_ai.get_class().get_name() if src_ai else 'NONE'}")

# Check if PlayerMesh has valid skeleton for retarget
print(f"Hidden skeleton: {mesh.get_editor_property('SkeletalMeshAsset').get_editor_property('skeleton').get_name()}")
pm_skel = pm.get_editor_property("SkeletalMeshAsset")
if pm_skel:
    print(f"Player skeleton: {pm_skel.get_editor_property('skeleton').get_name()}")

# List bones on player mesh to verify they exist
print(f"PlayerMesh bone count: {pm.get_num_bones()}")
print(f"HiddenMesh bone count: {mesh.get_num_bones()}")
