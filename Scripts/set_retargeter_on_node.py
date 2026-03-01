import unreal

abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
rtg = unreal.load_asset("/Game/gasp_Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin")

if abp and rtg:
    retarget_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_RetargetPoseFromMesh)
    if retarget_nodes:
        node = retarget_nodes[0]
        inner = node.get_editor_property("Node")
        print(f"Before: ik_retargeter_asset = {inner.get_editor_property('ik_retargeter_asset')}")

        # Set the retargeter asset
        inner.set_editor_property("ik_retargeter_asset", rtg)
        print(f"After: ik_retargeter_asset = {inner.get_editor_property('ik_retargeter_asset')}")

        # Write the modified struct back to the node
        node.set_editor_property("Node", inner)

        # Verify
        inner2 = node.get_editor_property("Node")
        print(f"Verify: ik_retargeter_asset = {inner2.get_editor_property('ik_retargeter_asset')}")

        # Save
        unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
        print("Saved ABP_GracePlayer")
    else:
        print("ERROR: No RetargetPoseFromMesh nodes found")
