import unreal

bs = unreal.load_asset('/Game/Core/Player/BS_Grace_Locomotion')
if not bs:
    print("ERROR: BS_Grace_Locomotion not found")
else:
    print(f"BlendSpace loaded: {type(bs).__name__}")

    idle = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop')
    walk = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F')
    run = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F')

    # Build sample data manually
    s1 = unreal.BlendSample()
    s1.set_editor_property('animation', idle)
    s1.set_editor_property('sample_value', unreal.Vector(0, 0, 0))

    s2 = unreal.BlendSample()
    s2.set_editor_property('animation', walk)
    s2.set_editor_property('sample_value', unreal.Vector(200, 0, 0))

    s3 = unreal.BlendSample()
    s3.set_editor_property('animation', run)
    s3.set_editor_property('sample_value', unreal.Vector(400, 0, 0))

    bs.set_editor_property('sample_data', [s1, s2, s3])
    print("Set 3 samples")

    unreal.EditorAssetLibrary.save_asset('/Game/Core/Player/BS_Grace_Locomotion')
    print("Saved")
