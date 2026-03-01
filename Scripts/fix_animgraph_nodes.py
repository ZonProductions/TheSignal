import unreal

abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
if not abp:
    print("ERROR: ABP not found")
    raise SystemExit

# Fix 1: LinkedAnimGraph — set InstanceClass
linked_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_LinkedAnimGraph)
if linked_nodes:
    node = linked_nodes[0]
    inner = node.get_editor_property("Node")

    tac_abp = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Animations/UE5/ABP_TacticalShooter_UE5/ABP_TacticalShooter_UE5")
    if tac_abp:
        tac_class = tac_abp.generated_class()
        print(f"Setting LinkedAnimGraph.InstanceClass to: {tac_class.get_name()}")
        inner.set_editor_property("instance_class", tac_class)
        node.set_editor_property("Node", inner)

        # Verify
        inner2 = node.get_editor_property("Node")
        ic = inner2.get_editor_property("instance_class")
        print(f"Verified LinkedAnimGraph.InstanceClass: {ic.get_name() if ic else 'NONE'}")
    else:
        print("ERROR: Could not load ABP_TacticalShooter_UE5")

# Fix 2: LayeredBoneBlend — set bone filter for spine_01
blend_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_LayeredBoneBlend)
if blend_nodes:
    node = blend_nodes[0]
    inner = node.get_editor_property("Node")

    layer_setup = inner.get_editor_property("layer_setup")
    if layer_setup:
        layer = layer_setup[0]
        bf = layer.get_editor_property("branch_filters")
        print(f"Current branch_filters count: {len(bf)}")

        # Inspect BranchFilter struct
        if bf:
            f0 = bf[0]
            f0_props = [p for p in dir(f0) if not p.startswith('_')]
            print(f"BranchFilter props: {f0_props}")
            try:
                bn = f0.get_editor_property("bone_name")
                print(f"  Current bone_name: {bn}")
                bd = f0.get_editor_property("blend_depth")
                print(f"  Current blend_depth: {bd}")
            except Exception as e:
                print(f"  Read error: {e}")

            # Set bone_name to spine_01
            f0.set_editor_property("bone_name", "spine_01")
            f0.set_editor_property("blend_depth", 0)
            bf[0] = f0
            layer.set_editor_property("branch_filters", bf)
            layer_setup[0] = layer
            inner.set_editor_property("layer_setup", layer_setup)
            node.set_editor_property("Node", inner)

            # Verify
            inner2 = node.get_editor_property("Node")
            ls2 = inner2.get_editor_property("layer_setup")
            bf2 = ls2[0].get_editor_property("branch_filters")
            bn2 = bf2[0].get_editor_property("bone_name")
            bd2 = bf2[0].get_editor_property("blend_depth")
            print(f"Verified: bone_name={bn2}, blend_depth={bd2}")

unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
print("Saved ABP_GracePlayer")
