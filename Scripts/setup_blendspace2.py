import unreal

# Delete the empty one and recreate with samples via a different approach
# First check if we can just open and manually configure it
bs = unreal.load_asset('/Game/Core/Player/BS_Grace_Locomotion')
print(f"Type: {type(bs).__name__}")
print(f"Num samples: {len(bs.get_editor_property('sample_data'))}")

# Try validate_sample_input approach
idle = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Idle/M_Neutral_Stand_Idle_Loop')
walk = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Walk/M_Neutral_Walk_Loop_F')
run = unreal.load_asset('/Game/gasp_Characters/UEFN_Mannequin/Animations/Run/M_Neutral_Run_Loop_F')

# Check all available methods on BlendSpace1D
methods = [m for m in dir(bs) if not m.startswith('_') and callable(getattr(bs, m, None))]
for m in methods:
    if any(k in m.lower() for k in ['sample', 'add', 'point', 'blend', 'edit']):
        print(f"  Method: {m}")
