import unreal

skeleton = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Meshes/SK_UEFN_Mannequin')

factory = unreal.BlendSpaceFactory1D()
factory.set_editor_property('target_skeleton', skeleton)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
bs = asset_tools.create_asset('BS_Grace_Locomotion', '/Game/Core/Player', None, factory)

if bs:
    idle = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop')
    walk = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F')
    run = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F')

    # BlendSpace1D uses sample_data array
    samples = bs.get_editor_property('sample_data')
    print(f"Type of sample_data: {type(samples)}")

    # Try adding via the blend parameter
    # Check available methods
    methods = [m for m in dir(bs) if not m.startswith('_') and 'sample' in m.lower()]
    print(f"Sample methods: {methods}")

    # Check blend parameter properties
    props = [m for m in dir(bs) if not m.startswith('_') and ('blend' in m.lower() or 'axis' in m.lower() or 'param' in m.lower())]
    print(f"Blend/param props: {props}")

    unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BS_Grace_Locomotion')
    print(f"Created: {bs.get_path_name()}")
else:
    print("ERROR: Failed to create blend space")
