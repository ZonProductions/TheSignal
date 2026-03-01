import unreal

abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")

# Set use_attached_parent = True on CopyPoseFromMesh
nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_CopyPoseFromMesh)
if nodes:
    inner = nodes[0].get_editor_property("Node")
    inner.set_editor_property("use_attached_parent", True)
    nodes[0].set_editor_property("Node", inner)
    print(f"CopyPoseFromMesh use_attached_parent: {nodes[0].get_editor_property('Node').get_editor_property('use_attached_parent')}")

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
print("Saved")
