import unreal

# Check 1: Is the retargeter still set on the node?
abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
if abp:
    nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_RetargetPoseFromMesh)
    if nodes:
        inner = nodes[0].get_editor_property("Node")
        rtg = inner.get_editor_property("ik_retargeter_asset")
        print(f"RetargetNode.IKRetargeter: {rtg.get_name() if rtg else 'NONE'}")
        src = inner.get_editor_property("source_mesh_component")
        print(f"RetargetNode.SourceMeshComponent: {src}")
        print(f"RetargetNode.use_attached_parent: {inner.get_editor_property('use_attached_parent')}")
    else:
        print("No RetargetPoseFromMesh nodes!")

# Check 2: LinkedAnimGraph node state
    linked_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_LinkedAnimGraph)
    print(f"LinkedAnimGraph nodes: {len(linked_nodes)}")
    for n in linked_nodes:
        inner = n.get_editor_property("Node")
        inner_props = [p for p in dir(inner) if not p.startswith('_')]
        print(f"  LinkedAnimGraph inner props: {inner_props}")
        try:
            ic = inner.get_editor_property("instance_class")
            print(f"  InstanceClass: {ic}")
        except Exception as e:
            print(f"  InstanceClass error: {e}")

# Check 3: LayeredBoneBlend config
    blend_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_LayeredBoneBlend)
    print(f"LayeredBoneBlend nodes: {len(blend_nodes)}")
    for n in blend_nodes:
        inner = n.get_editor_property("Node")
        inner_props = [p for p in dir(inner) if not p.startswith('_')]
        print(f"  LBB inner props: {inner_props}")
        try:
            bf = inner.get_editor_property("layer_setup")
            print(f"  LayerSetup: {bf}")
        except:
            pass
        try:
            bf = inner.get_editor_property("branch_filters")
            print(f"  BranchFilters: {bf}")
        except:
            pass
