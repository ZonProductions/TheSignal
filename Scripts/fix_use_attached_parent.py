import unreal

abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_RetargetPoseFromMesh)
if nodes:
    inner = nodes[0].get_editor_property("Node")
    print(f"Before: use_attached_parent = {inner.get_editor_property('use_attached_parent')}")

    inner.set_editor_property("use_attached_parent", False)
    nodes[0].set_editor_property("Node", inner)

    # Verify
    inner2 = nodes[0].get_editor_property("Node")
    print(f"After: use_attached_parent = {inner2.get_editor_property('use_attached_parent')}")

    unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
    print("Saved ABP_GracePlayer")
