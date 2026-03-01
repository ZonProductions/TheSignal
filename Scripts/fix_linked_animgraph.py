import unreal

abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
tac_abp = unreal.load_asset("/Game/KINEMATION/TacticalShooterPack/Animations/UE5/ABP_TacticalShooter_UE5")

if abp and tac_abp:
    tac_class = tac_abp.generated_class()
    print(f"TacticalShooter class: {tac_class.get_name()}")

    linked_nodes = abp.get_nodes_of_class(unreal.AnimGraphNode_LinkedAnimGraph)
    if linked_nodes:
        node = linked_nodes[0]
        inner = node.get_editor_property("Node")
        inner.set_editor_property("instance_class", tac_class)
        node.set_editor_property("Node", inner)

        # Verify
        inner2 = node.get_editor_property("Node")
        ic = inner2.get_editor_property("instance_class")
        print(f"Verified: InstanceClass = {ic.get_name() if ic else 'NONE'}")

        unreal.EditorAssetLibrary.save_asset("/Game/Core/Player/ABP_GracePlayer")
        print("Saved")
    else:
        print("No LinkedAnimGraph nodes")
else:
    print(f"Load failed: abp={abp}, tac_abp={tac_abp}")
