import unreal

# Load the ABP
abp = unreal.load_asset("/Game/Core/Player/ABP_GracePlayer")
print(f"ABP: {abp}")

# Load the IK Retargeter
rtg = unreal.load_asset("/Game/gasp_Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin")
print(f"Retargeter: {rtg}")

if abp and rtg:
    # Get the AnimGraph
    graphs = abp.get_editor_property("function_graphs")
    print(f"Num graphs: {len(graphs) if graphs else 0}")

    # Try to find the RetargetPoseFromMesh node
    # AnimBP graphs are accessed differently
    # Let's look at all available properties
    props = [p for p in dir(abp) if not p.startswith('_') and 'graph' in p.lower()]
    print(f"Graph-related props: {props}")
