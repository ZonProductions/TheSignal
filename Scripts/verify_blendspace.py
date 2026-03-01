import unreal

bs = unreal.load_asset('/Game/Core/Player/BS_Grace_Locomotion')
samples = bs.get_editor_property('sample_data')
for i, s in enumerate(samples):
    anim = s.get_editor_property('animation')
    val = s.get_editor_property('sample_value')
    anim_name = anim.get_name() if anim else "NONE"
    print(f"Sample {i}: {anim_name} @ {val}")

# Check blend parameter
bp = bs.get_editor_property('blend_parameter')
print(f"Blend param: display={bp.get_editor_property('display_name')}, min={bp.get_editor_property('min')}, max={bp.get_editor_property('max')}")
