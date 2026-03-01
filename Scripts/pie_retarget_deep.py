import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Character)[0]
mesh = a.get_editor_property("Mesh")
pm = a.get_editor_property("PlayerMesh")

# Verify use_attached_parent actually persisted
abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_RetargetPoseFromMesh)
inner = nodes[0].get_editor_property("Node")
print(f"use_attached_parent: {inner.get_editor_property('use_attached_parent')}")
print(f"ik_retargeter_asset: {inner.get_editor_property('ik_retargeter_asset')}")

# Check runtime AnimInstance state
pai = pm.get_anim_instance()
print(f"PlayerMesh AnimInst: {pai.get_class().get_name()}")
src = pai.get_editor_property("SourceMeshComponent")
print(f"SourceMeshComponent: {src.get_name() if src else 'NONE'}")

# Force walk on hidden mesh again (lost on PIE restart)
walk = unreal.load_asset("/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F")
ai = mesh.get_anim_instance()
ai.set_animation_asset(walk)
ai.set_looping(True)
ai.set_playing(True)
print("Forced walk on hidden mesh")

# Check if hidden mesh bones have same names as what retargeter expects
print(f"\nHidden mesh bones (first 15):")
for i in range(min(15, mesh.get_num_bones())):
    name = mesh.get_bone_name(i)
    print(f"  {i}: {name}")

print(f"\nPlayer mesh bones (first 15):")
for i in range(min(15, pm.get_num_bones())):
    name = pm.get_bone_name(i)
    print(f"  {i}: {name}")
